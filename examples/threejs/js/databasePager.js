// databasePager.js -- a deliberately lightweight stand-in for osg's
// DatabasePager / VSG's DatabasePager.
//
// It does not try to reproduce osgDB's threaded load-and-merge machinery. Each
// frame it walks the registered PagedLOD nodes and asks two questions:
//
//   1. is the node's bounding sphere in front of the camera and inside the
//      view frustum?  (i.e. would the tile be visible at all)
//   2. does the sphere cover enough of the screen to justify the next level?
//
// When both are true the child file is fetched, read through the WASM reader,
// converted with the same scene builder as the root, and attached to the node;
// the in-memory stand-in (osg's children[1]) is then hidden. New PagedLOD nodes
// found in the loaded file are registered as well, so paging continues down the
// tree as the camera moves closer.
//
// Nodes that stay out of view (or below the detail threshold) for longer than
// `expiryDelay` are detached again and their geometry/materials/textures are
// disposed. The raw file bytes are kept in a small byte-budget LRU cache, so
// coming back to a tile re-parses locally instead of hitting the network.

import * as THREE from 'three';
import { buildScene, releaseTexture } from './sceneBuilder.js';

const _frustum = new THREE.Frustum();
const _projScreen = new THREE.Matrix4();
const _sphere = new THREE.Sphere();
const _center = new THREE.Vector3();
const _origin = new THREE.Vector3();
const MATERIAL_MAPS = ['map', 'alphaMap', 'emissiveMap', 'normalMap', 'specularMap'];

export class DatabasePager {
    constructor(reader, options = {}) {
        this.reader = reader;
        this.builderOptions = options.builderOptions || {};

        // Minimum screen-height ratio at which a child becomes worth loading.
        // `rangeRatio` uses osg's own range instead (see below) when available.
        this.ratio = options.ratio !== undefined ? options.ratio : 0.25;
        this.useOsgRange = options.useOsgRange !== false;
        this.maxConcurrent = options.maxConcurrent || 4;
        this.maxPerFrame = options.maxPerFrame || 2;

        // A loaded tile is dropped once it has not been "needed" for this many
        // seconds. `unloadFactor` widens the keep-alive test relative to the
        // load threshold, so a tile sitting exactly on the boundary does not
        // load/unload every frame.
        this.expiryDelay = options.expiryDelay !== undefined ? options.expiryDelay : 5.0;
        this.unloadFactor = options.unloadFactor !== undefined ? options.unloadFactor : 0.5;

        // Raw (still compressed) file bytes, keyed by URL. Unloaded tiles can
        // therefore be rebuilt without another network round trip.
        this.cacheLimit = options.cacheLimit !== undefined ? options.cacheLimit : 64 * 1024 * 1024;
        this.cache = new Map();
        this.cacheBytes = 0;

        this.entries = [];
        this.pending = 0;
        this.stats = { registered: 0, queued: 0, loaded: 0, unloaded: 0, failed: 0,
                       pending: 0, cacheHits: 0, cacheSize: 0, cacheBytes: 0 };
    }

    // Collect the PagedLOD nodes of a (sub)tree.
    register(root) {
        if (!root) return;
        root.traverse((n) => {
            const ud = n.userData;
            if (ud && ud.paged && !ud.registered) {
                ud.registered = true;
                this.entries.push(n);
                this.stats.registered++;
            }
        });
    }

    get fovTan() {
        // tan(fovY / 2) of the active camera, refreshed in update().
        return this._fovTan || 0.5;
    }

    update(camera) {
        const now = performance.now();
        this._fovTan = Math.tan(THREE.MathUtils.degToRad(camera.fov) * 0.5);
        _projScreen.multiplyMatrices(camera.projectionMatrix, camera.matrixWorldInverse);
        _frustum.setFromProjectionMatrix(_projScreen);

        // Snapshot: unload() rebuilds this.entries, and the loop must not be
        // confused by that.
        const entries = this.entries;
        let started = 0;
        for (let i = entries.length - 1; i >= 0; --i) {
            const node = entries[i];
            const ud = node.userData;
            if (!ud || ud.failed) continue;

            _center.copy(ud.center || _origin).applyMatrix4(node.matrixWorld);
            const radius = ud.radius || 1;
            _sphere.set(_center, radius);

            // 1. visibility: anything outside the frustum is neither loaded
            //    nor kept alive.
            const visible = _frustum.intersectsSphere(_sphere);
            const ratio = radius / Math.max(camera.position.distanceTo(_center), 1e-6) / this.fovTan;

            if (ud.loaded) {
                // 2. keep-alive: a little more tolerant than the load test, and
                //    only after the tile has been unneeded for a while.
                const keep = visible && ratio >= this.thresholdFor(ud) * this.unloadFactor;
                if (keep) {
                    ud.lastNeeded = now;
                } else if (now - (ud.lastNeeded || now) >= this.expiryDelay * 1000) {
                    if (!hasPendingLoad(ud.child)) this.unload(node);
                }
                continue;
            }

            if (ud.loading || !ud.file) continue;
            if (started >= this.maxPerFrame || this.pending >= this.maxConcurrent) continue;
            if (!visible) continue;
            if (ratio < this.thresholdFor(ud)) continue;

            ud.lastNeeded = now;
            this.startLoad(node);
            ++started;
        }
        this.stats.pending = this.pending;
        this.stats.queued = this.entries.filter((n) => !n.userData.loaded && !n.userData.failed && n.userData.file).length;
        this.stats.cacheSize = this.cache.size;
        this.stats.cacheBytes = this.cacheBytes;
    }

    // osg stores the switch as an eye distance in units of the bounding radius:
    // the child shows while distance <= radius * maxRange, which in screen terms
    // is ratio >= 1 / (maxRange * tan(fov/2)).
    thresholdFor(ud) {
        if (this.useOsgRange && ud.range > 0) return 1.0 / (ud.range * this.fovTan);
        return this.ratio;
    }

    startLoad(node) {
        const ud = node.userData;
        ud.loading = true;
        ++this.pending;
        this.load(node).catch((err) => {
            console.warn(`paged tile failed: ${ud.file}`, err);
            ud.failed = true;
            ++this.stats.failed;
        }).finally(() => {
            ud.loading = false;
            --this.pending;
        });
    }

    async load(node) {
        const ud = node.userData;
        const url = resolveAgainst(this.builderOptions.baseUrl, ud.file);
        if (!url) return;

        // Parse from the cached bytes when possible: the cache removes the
        // network round trip, not the (cheap) WASM parse.
        const bytes = await this.fetchBytes(url);
        const ir = this.reader.read(bytes);
        if (!ir) throw new Error('reader returned no IR');

        const options = Object.assign({}, this.builderOptions, { baseUrl: url });
        const builder = await buildScene(ir, options);
        const child = builder.root;
        if (!child) throw new Error('empty scene');

        node.add(child);
        if (ud.fallback) ud.fallback.visible = false;
        ud.child = child;
        ud.loaded = true;
        ud.lastNeeded = performance.now();
        ++this.stats.loaded;

        // The freshly loaded tile may reference tiles of its own.
        this.register(child);
    }

    // Detach a resident tile and give its GPU resources back. The node itself
    // stays registered, so it can be loaded again later.
    unload(node) {
        const ud = node.userData;
        const child = ud.child;
        if (child) {
            this.unregister(child);
            node.remove(child);
            disposeTree(child);
            ud.child = null;
        }
        ud.loaded = false;
        if (ud.fallback) ud.fallback.visible = true;
        ++this.stats.unloaded;
        this.stats.loaded = Math.max(0, this.stats.loaded - 1);
    }

    // Forget the PagedLOD nodes inside a subtree (they are about to disappear
    // from the scene) and make them registrable again.
    unregister(root) {
        const dropped = new Set();
        root.traverse((n) => {
            const ud = n.userData;
            if (ud && ud.paged && ud.registered) {
                ud.registered = false;
                dropped.add(n);
            }
        });
        if (!dropped.size) return;
        this.entries = this.entries.filter((n) => !dropped.has(n));
        this.stats.registered -= dropped.size;
    }

    // URL -> raw bytes, with an LRU eviction on a byte budget.
    async fetchBytes(url) {
        const cached = this.cache.get(url);
        if (cached) {
            this.cache.delete(url);           // refresh LRU position
            this.cache.set(url, cached);
            ++this.stats.cacheHits;
            return cached;
        }
        const response = await fetch(url);
        if (!response.ok)
            throw new Error(`${response.status} ${response.statusText} for ${url}`);
        const bytes = new Uint8Array(await response.arrayBuffer());
        this.cachePut(url, bytes);
        return bytes;
    }

    cachePut(url, bytes) {
        if (!bytes.byteLength || bytes.byteLength > this.cacheLimit) return;
        const prev = this.cache.get(url);
        if (prev) this.cacheBytes -= prev.byteLength;
        this.cache.set(url, bytes);
        this.cacheBytes += bytes.byteLength;
        while (this.cacheBytes > this.cacheLimit && this.cache.size) {
            const oldest = this.cache.keys().next().value;
            this.cacheBytes -= this.cache.get(oldest).byteLength;
            this.cache.delete(oldest);
        }
    }
}

// Release the GPU-side resources of an unloaded subtree. Textures are also
// removed from the builder's flip-tracking sets (see sceneBuilder.releaseTexture).
function disposeTree(root) {
    root.traverse((n) => {
        if (n.geometry) n.geometry.dispose();
        const materials = Array.isArray(n.material) ? n.material : [n.material];
        for (const material of materials) {
            if (!material) continue;
            for (const slot of MATERIAL_MAPS) {
                if (material[slot]) releaseTexture(material[slot]);
            }
            material.dispose();
        }
    });
}

// A tile that is still streaming must not be pulled out from under its own
// promise callbacks.
function hasPendingLoad(root) {
    let pending = false;
    if (root) root.traverse((n) => { if (n.userData && n.userData.loading) pending = true; });
    return pending;
}

// osg stores child file names relative to the tile that references them, so the
// paged child must be fetched next to its parent, not at the page root.
function resolveAgainst(baseUrl, name) {
    if (!name) return null;
    const clean = name.replace(/\\/g, '/');
    if (/^[a-z]+:\/\//i.test(clean) || clean.startsWith('/')) return clean;
    if (!baseUrl) return clean;
    return baseUrl.replace(/\/[^/]*$/, '/') + clean;
}
