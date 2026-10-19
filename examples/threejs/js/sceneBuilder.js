// sceneBuilder.js -- replay the neutral SceneIR into a three.js scene graph.
//
// This is the JavaScript counterpart of examples/vsg/vsg_scene_builder.h: the
// same command stream, the same classification rules, only the target engine
// differs. It is intentionally small: three.js already provides BufferGeometry,
// MeshPhongMaterial, DataTexture, ... so the adapter mostly just routes the
// recorded osg commands into three's constructors.
//
//   osg::Group/Geode/Node        -> THREE.Group
//   osg::MatrixTransform ...     -> THREE.Group + Matrix command
//   osg::LOD                     -> THREE.Group (+ range bookkeeping)
//   osg::PagedLOD / osg::ProxyNode -> PagedLOD node (see databasePager.js)
//   osg::Switch                  -> THREE.Group
//   osg::Geometry                -> THREE.Group holding Meshes
//   osg::Image                   -> THREE.Texture
//   osg::Material                -> MeshPhongMaterial parameters

import * as THREE from 'three';

const GL = {
    POINTS: 0, LINES: 1, LINE_LOOP: 2, LINE_STRIP: 3, TRIANGLES: 4,
    TRIANGLE_STRIP: 5, TRIANGLE_FAN: 6, QUADS: 7, QUAD_STRIP: 8, POLYGON: 9
};

// IRArg::Kind (see SceneIR.h)
const K_OBJECT = 6, K_MATRIX = 12, K_VEC4 = 9, K_VEC3 = 8;

const argNum = (args, i, d = 0) => {
    const a = args[i];
    if (!a) return d;
    return (a.n !== undefined) ? a.n : (a.v ? a.v[0] : d);
};
const argStr = (args, i, d = '') => {
    const a = args[i];
    if (!a) return d;
    return (a.s !== undefined) ? a.s : d;
};
const argRef = (args, i) => {
    const a = args[i];
    if (!a || a.k !== K_OBJECT) return -1;
    return (a.o !== undefined) ? a.o : -1;
};
const argVec = (args, i) => {
    const a = args[i];
    return (a && a.v) ? a.v : null;
};

const has = (s, sub) => s.indexOf(sub) !== -1;

// Mirrors VsgSceneBuilder::classify -- order matters (PagedLOD contains "LOD",
// the Draw* names contain "Array", ...).
function classify(c) {
    if (c === 'osg::Geometry') return 'geometry';
    if (has(c, 'PagedLOD')) return 'pagedlod';
    if (c === 'osg::ProxyNode') return 'pagedlod';
    if (has(c, 'LOD')) return 'lod';
    if (c === 'osg::Switch') return 'switch';
    if (has(c, 'DrawArray') || has(c, 'DrawElements') || c === 'osg::PrimitiveSet') return 'primitive';
    if (has(c, 'Array')) return 'array';
    if (c === 'osg::Image') return 'image';
    if (c === 'osg::StateSet') return 'stateset';
    if (has(c, 'Texture')) return 'texture';
    if (c === 'osg::Material') return 'material';
    if (has(c, 'Transform') || c === 'osg::Camera') return 'transform';
    if (has(c, 'Node') || has(c, 'Group') || has(c, 'Geode') ||
        c === 'osg::Billboard' || c === 'osg::Sequence') return 'group';
    return 'generic';
}

// A paged node: a plain Group whose userData tells databasePager.js what to
// stream in. children[0] of osg's PagedLOD (external file) becomes `files`,
// children[1] (the in-memory stand-in) becomes `fallback`.
export class PagedLOD extends THREE.Group {
    constructor(index) {
        super();
        this.name = 'PagedLOD';
        this.userData = {
            irIndex: index, paged: true,
            files: [], range: null, center: null, radius: 0,
            fallback: null, child: null, loaded: false
        };
    }
}

export async function buildScene(ir, options = {}) {
    const builder = new SceneBuilder(options);
    await builder.prepareImages(ir);
    for (let i = 0; i < ir.events.length; ++i) {
        const e = ir.events[i];
        const obj = ir.objects[e.object];
        if (!obj) continue;
        if (e.type === 0) builder.onObject(e.object, obj);
        else builder.onCommand(e.object, obj, e.name, e.args || []);
    }
    builder.finish();
    return builder;
}

class SceneBuilder {
    constructor(options) {
        this.options = options;
        // osg's texture coordinates put v = 0 at the bottom of the picture, and
        // osg images are stored bottom-up. three's default flipY = true is the
        // matching convention, so decoded files are flipped on upload.
        this.flipY = (options.flipY === undefined) ? true : !!options.flipY;
        this.root = null;
        this.nodes = [];          // ir index -> THREE.Object3D
        this.arrays = new Map();  // ir index -> { components, data: Float32Array }
        this.images = new Map();  // ir index -> THREE.Texture (or null)
        this.prims = new Map();   // ir index -> primitive state
        this.stateSets = new Map();   // ir index -> { texture, material }
        this.textures = new Map();    // ir index -> { image, wrapS, wrapT, ... }
        this.materials = new Map();   // ir index -> material parameters
        this.geoms = new Map();       // ir index -> geometry state
        this.lods = new Map();        // ir index -> lod state
        this.pagedNodes = [];         // PagedLOD nodes for the pager
        this.textureCache = new Map();  // url / data key -> THREE.Texture
        this.bounds = null;           // { center, radius } in the file's own space
        this.pendingDecodes = [];     // image decodes awaited before the scene is shown
    }

    // Decode every image of the file before the graph is built, so a tile never
    // appears with the white placeholder and then swaps its texture in. External
    // files (IMAGE_USE_FILE) are left to load in the background.
    async prepareImages(ir) {
        for (const obj of ir.objects) {
            if (obj.im) this.buildImage(obj);
        }
        if (this.pendingDecodes.length) await Promise.all(this.pendingDecodes);
        this.pendingDecodes.length = 0;
    }

    // -----------------------------------------------------------------------
    // object creation
    // -----------------------------------------------------------------------
    onObject(index, obj) {
        const kind = classify(obj.className);
        let node = null;

        switch (kind) {
        case 'transform':
        case 'switch':
        case 'group':
        case 'generic':
        case 'geometry':
            node = new THREE.Group();
            break;
        case 'lod':
        case 'pagedlod':
            node = new PagedLOD(index);
            break;
        default:
            node = null;   // data records (array / image / primitive / stateset / ...)
            break;
        }

        this.nodes[index] = node;
        if (index === 0 && node) this.root = node;

        if (kind === 'geometry') this.geoms.set(index, { node, vertex: -1, normal: -1, color: -1, texCoord: -1, stateSet: -1, primitives: [] });
        else if (kind === 'array') this.arrays.set(index, this.buildArray(obj));
        else if (kind === 'image') this.images.set(index, this.buildImage(obj));
        else if (kind === 'primitive') this.prims.set(index, this.buildPrimitive(obj));
        else if (kind === 'stateset') this.stateSets.set(index, { texture: -1, material: -1 });
        else if (kind === 'texture') this.textures.set(index, { image: -1 });
        else if (kind === 'material') this.materials.set(index, this.defaultMaterial());
        else if (kind === 'lod' || kind === 'pagedlod') {
            this.lods.set(index, { paged: kind === 'pagedlod', node, children: [], files: {}, ranges: {} });
        }
    }

    // -----------------------------------------------------------------------
    // command dispatch
    // -----------------------------------------------------------------------
    onCommand(index, obj, name, args) {
        switch (name) {
        case 'addChild': return this.addChild(this.nodes[index], argRef(args, 0), index);
        case 'addDrawable': return this.addChild(this.nodes[index], argRef(args, 0), index);
        case 'Matrix': return this.setMatrix(this.nodes[index], args);
        case 'VertexArray': case 'setVertexArray': return this.setGeomArray(index, 'vertex', argRef(args, 0));
        case 'NormalArray': case 'setNormalArray': return this.setGeomArray(index, 'normal', argRef(args, 0));
        case 'ColorArray': case 'setColorArray': return this.setGeomArray(index, 'color', argRef(args, 0));
        case 'setTexCoordArray': return this.setGeomArray(index, 'texCoord', argRef(args, 1));
        case 'TexCoordArrayList': return this.setGeomArray(index, 'texCoord', argRef(args, 0));
        case 'PrimitiveSetList':
            for (const a of args) {
                const g = this.geoms.get(index);
                if (g && a.o !== undefined && a.o >= 0) g.primitives.push(a.o);
            }
            return;
        case 'Mode': case 'First': case 'Count': {
            const p = this.prims.get(index);
            if (!p) return;
            if (name === 'Mode') p.mode = argNum(args, 0, p.mode);
            else if (name === 'First') p.first = argNum(args, 0, p.first);
            else p.count = argNum(args, 0, p.count);
            return;
        }
        case 'StateSet': case 'setStateSet': {
            const g = this.geoms.get(index);
            if (g) g.stateSet = argRef(args, 0);
            return;
        }
        case 'setTextureAttribute': {
            const ss = this.stateSets.get(index);
            if (!ss || args.length < 2) return;
            const unit = argNum(args, 0);
            if (unit === 0 && args[1].t && has(args[1].t, 'Texture')) ss.texture = argRef(args, 1);
            return;
        }
        case 'setAttribute': {
            const ss = this.stateSets.get(index);
            if (!ss || !args.length || !args[0].t) return;
            if (args[0].t === 'osg::Material') ss.material = argRef(args, 0);
            return;
        }
        case 'Image': {
            const t = this.textures.get(index);
            if (t) t.image = argRef(args, 0);
            return;
        }
        case 'setWrap': case 'WrapS': case 'setTextureWrap':
            this.setTextureWrap(index, args);
            return;
        case 'setAmbient': case 'setDiffuse': case 'setSpecular': case 'setEmission': case 'setShininess':
            this.setMaterial(index, name, args);
            return;
        case 'setFileName': {
            const l = this.lods.get(index);
            if (l && args.length >= 2) l.files[argNum(args, 0)] = argStr(args, 1);
            return;
        }
        case 'setRange': {
            const l = this.lods.get(index);
            if (l && args.length >= 3) l.ranges[argNum(args, 0)] = [argNum(args, 1), argNum(args, 2)];
            return;
        }
        case 'setCenter': {
            const l = this.lods.get(index);
            const v = argVec(args, 0);
            if (v && v.length >= 3) {
                const center = new THREE.Vector3(v[0], v[1], v[2]);
                if (l) l.center = center;
                const node = this.nodes[index];
                if (node) node.userData.center = center;
            }
            return;
        }
        case 'setRadius': {
            const l = this.lods.get(index);
            if (l) l.radius = argNum(args, 0);
            const node = this.nodes[index];
            if (node) node.userData.radius = argNum(args, 0);
            return;
        }
        case 'setInitialBound': {
            // osg writes a node's explicitly-set bound as a BoundingSphere
            // (BSPHERE: x,y,z,radius) or a BoundingBox (BBOX: min xyz, max xyz).
            const node = this.nodes[index];
            const v = argVec(args, 0);
            if (node && v) {
                if (args[0].k === 13 && v.length >= 6) {
                    const c = [(v[0] + v[3]) / 2, (v[1] + v[4]) / 2, (v[2] + v[5]) / 2];
                    const dx = v[3] - v[0], dy = v[4] - v[1], dz = v[5] - v[2];
                    node.userData.center = new THREE.Vector3(c[0], c[1], c[2]);
                    node.userData.radius = 0.5 * Math.sqrt(dx * dx + dy * dy + dz * dz);
                } else if (v.length >= 4) {
                    node.userData.center = new THREE.Vector3(v[0], v[1], v[2]);
                    node.userData.radius = v[3];
                }
            }
            return;
        }
        default:
            return;   // unhandled commands (Program/Shader/TexGen/...) are ignored
        }
    }

    addChild(parent, childIndex, parentIndex) {
        const child = (childIndex >= 0) ? this.nodes[childIndex] : null;
        if (!child) return;
        const lod = this.lods.get(parentIndex);
        if (lod) { lod.children.push(child); return; }   // assembled in finish()
        if (parent) parent.add(child);
    }

    setMatrix(node, args) {
        const m = argVec(args, 0);
        if (!node || !m || m.length < 16) return;
        // osg stores row-major with row-vector math; three (like OpenGL) reads
        // column-major, which is exactly the transpose this needs.
        const mat = new THREE.Matrix4();
        mat.fromArray(m);
        mat.decompose(node.position, node.quaternion, node.scale);
        node.updateMatrix();
    }

    setGeomArray(index, slot, arrayIndex) {
        const g = this.geoms.get(index);
        if (g && arrayIndex >= 0) g[slot] = arrayIndex;
    }

    setTextureWrap(index, args) {
        const t = this.textures.get(index);
        if (!t) return;
        const token = argStr(args, args.length - 1);
        if (token) t.wrap = token;
    }

    setMaterial(index, name, args) {
        const m = this.materials.get(index);
        if (!m || !args.length) return;
        if (name === 'setShininess') { m.shininess = argNum(args, args.length - 1); return; }
        const c = argVec(args, args.length - 1);
        if (!c || c.length < 4) return;
        const col = new THREE.Color(c[0], c[1], c[2]);
        if (name === 'setAmbient') m.ambient = col;
        else if (name === 'setDiffuse') m.diffuse = col;
        else if (name === 'setSpecular') m.specular = col;
        else m.emissive = col;
    }

    defaultMaterial() {
        return {
            ambient: new THREE.Color(0.2, 0.2, 0.2),
            diffuse: new THREE.Color(0.8, 0.8, 0.8),
            specular: new THREE.Color(0.0, 0.0, 0.0),
            emissive: new THREE.Color(0.0, 0.0, 0.0),
            shininess: 0
        };
    }

    // -----------------------------------------------------------------------
    // data records
    // -----------------------------------------------------------------------
    buildArray(obj) {
        const vec = obj.vec;
        if (!vec || !vec.data || !vec.data.length) return null;
        const comps = Math.max(1, vec.components | 0);
        const src = vec.data;
        const data = new Float32Array(src.length);
        // Integral element types (Vec4ubArray, UShortArray, ...) come back as
        // their raw numeric range; colours are the only consumer that needs the
        // normalised form, and three normalises Uint8 attributes itself, so keep
        // the values verbatim and let the attribute type decide.
        for (let i = 0; i < src.length; ++i) data[i] = src[i];
        return { components: comps, data };
    }

    buildPrimitive(obj) {
        const p = obj.p || {};
        let indices = null;
        if (p.indices && p.indices.length) indices = p.indices;
        else if (obj.vec && obj.vec.data && obj.vec.data.length) {
            const src = obj.vec.data;
            indices = new Uint32Array(src.length);
            for (let i = 0; i < src.length; ++i) indices[i] = src[i];
        }
        return {
            mode: p.mode | 0,
            first: p.first | 0,
            count: p.count | 0,
            indices: (indices && indices.length) ? indices : null,
            lengths: (p.lengths && p.lengths.length) ? p.lengths : null
        };
    }

    buildImage(obj) {
        const im = obj.im;
        if (!im) return null;
        const key = `${im.decision}:${im.fileName}:${im.width}x${im.height}:${im.pixels ? im.pixels.length : 0}`;
        if (this.textureCache.has(key)) return this.textureCache.get(key);

        let texture = null;
        if (im.pixels && im.pixels.length) {
            // decision 0 = raw pixels in osg's layout, 1 = embedded encoded file
            // (the .osgb "inline file" storage). Anything else with bytes is
            // treated as an encoded file too.
            if (im.decision === 0) texture = this.textureFromPixels(im);
            else texture = this.textureFromBlob(new Blob([im.pixels]), im.fileName, this.flipY);
        } else if (im.fileName) {
            const url = this.resolveUrl(im.fileName);
            if (url) texture = this.textureFromUrl(url, this.flipY);
        }

        this.textureCache.set(key, texture);
        return texture;
    }

    resolveUrl(name) {
        const clean = name.replace(/\\/g, '/');
        if (/^[a-z]+:\/\//i.test(clean) || clean.startsWith('/')) return clean;
        if (!this.options.baseUrl) return clean;
        // Keep the whole relative path: osg names external images relative to the
        // file that references them (e.g. "Images/clockface.JPG").
        return this.options.baseUrl.replace(/\/[^/]*$/, '/') + clean;
    }

    textureFromUrl(url, flipY) {
        const tex = new THREE.Texture();
        tex.flipY = flipY;
        tex.colorSpace = THREE.SRGBColorSpace;
        tex.userData.placeholder = true;   // cleared once the real image arrives
        tex.image = whitePixel();          // placeholder until (or unless) it loads
        tex.needsUpdate = true;
        imageTextures.add(tex);            // flipY works for <img> sources
        new THREE.TextureLoader().load(url, (loaded) => {
            tex.image = loaded.image;
            tex.userData.placeholder = false;
            tex.needsUpdate = true;
        }, undefined, () => { /* keep the white placeholder */ });
        return tex;
    }

    textureFromBlob(blob, hint, flipY) {
        const tex = new THREE.Texture();
        tex.colorSpace = THREE.SRGBColorSpace;
        tex.image = whitePixel();          // placeholder until the decode finishes
        tex.needsUpdate = true;
        bitmapTextures.add({ tex, blob });
        this.pendingDecodes.push(decodeBitmap(tex, blob, flipY));
        return tex;
    }

    textureFromPixels(im) {
        const w = im.width | 0, h = im.height | 0;
        if (!w || !h) return null;
        const GL_UNSIGNED_BYTE = 0x1401, GL_RGB = 0x1907, GL_RGBA = 0x1908;
        if (im.dataType !== GL_UNSIGNED_BYTE) return null;
        if (im.pixelFormat !== GL_RGB && im.pixelFormat !== GL_RGBA) return null;

        const comps = (im.pixelFormat === GL_RGBA) ? 4 : 3;
        const src = im.pixels;
        const rgba = new Uint8Array(w * h * 4);
        for (let i = 0, j = 0; i < w * h; ++i, j += comps) {
            rgba[i * 4 + 0] = src[j];
            rgba[i * 4 + 1] = src[j + 1];
            rgba[i * 4 + 2] = src[j + 2];
            rgba[i * 4 + 3] = (comps === 4) ? src[j + 3] : 255;
        }
        const tex = new THREE.DataTexture(rgba, w, h, THREE.RGBAFormat);
        // osg hands the rows over bottom-up; three must not flip them again.
        tex.flipY = false;
        tex.colorSpace = THREE.SRGBColorSpace;
        tex.magFilter = THREE.LinearFilter;
        tex.minFilter = THREE.LinearMipmapLinearFilter;
        tex.generateMipmaps = true;
        tex.needsUpdate = true;
        return tex;
    }

    textureForStateSet(index) {
        const ss = this.stateSets.get(index);
        if (!ss || ss.texture < 0) return null;
        const t = this.textures.get(ss.texture);
        if (!t || t.image < 0) return null;
        return this.images.get(t.image) || null;
    }

    materialForStateSet(index) {
        const ss = (index >= 0) ? this.stateSets.get(index) : null;
        let params = null;
        if (ss && ss.material >= 0) params = this.materials.get(ss.material);
        if (!params && this.materials.size) params = this.materials.values().next().value;
        return params || this.defaultMaterial();
    }

    // -----------------------------------------------------------------------
    // finish: build the draws and assemble the LOD / PagedLOD nodes
    // -----------------------------------------------------------------------
    finish() {
        for (const [index, g] of this.geoms) this.buildGeometry(index, g);
        for (const [, lod] of this.lods) this.buildLod(lod);

        // Register the paging metadata (world-space bounds) for the pager.
        for (const node of this.pagedNodes) this.finalizePagedBounds(node);

        this.computeBounds();
    }

    // The scene's bounding sphere in the file's own coordinate system. osg's
    // PagedLOD/LOD carry an explicit center+radius (and other nodes may store an
    // initial bound), which is authoritative and does not depend on how much of
    // the database has been paged in yet. Only when neither is present fall back
    // to the geometry that was actually built.
    computeBounds() {
        const root = this.nodes[0];
        const ud = root && root.userData;
        if (ud && ud.center && ud.radius > 0) {
            this.bounds = { center: ud.center.clone(), radius: ud.radius };
            return;
        }
        if (!root) return;
        root.updateMatrixWorld(true);
        const box = new THREE.Box3().setFromObject(root);
        if (box.isEmpty()) return;
        const sphere = box.getBoundingSphere(new THREE.Sphere());
        this.bounds = { center: sphere.center.clone(), radius: sphere.radius };
    }

    getAttribute(index, components) {
        const a = this.arrays.get(index);
        if (!a) return null;
        return new THREE.Float32BufferAttribute(a.data, a.components || components);
    }

    buildGeometry(index, g) {
        if (!g.node || g.vertex < 0) return;
        const position = this.getAttribute(g.vertex, 3);
        if (!position) return;

        const vertexCount = position.count;
        const texture = this.textureForStateSet(g.stateSet);
        const params = this.materialForStateSet(g.stateSet);

        // Expand every primitive set into plain triangle/line/point lists.
        const groups = new Map();
        if (!g.primitives.length) {
            const list = [];
            for (let i = 0; i < vertexCount; ++i) list.push(i);
            groups.set('triangles', list);
        } else {
            for (const pi of g.primitives) {
                const p = this.prims.get(pi);
                if (p) this.appendPrimitive(p, groups);
            }
        }
        if (!groups.size) return;

        let normal = (g.normal >= 0) ? this.getAttribute(g.normal, 3) : null;
        let color = (g.color >= 0) ? this.getAttribute(g.color, 4) : null;
        let texCoord = (g.texCoord >= 0) ? this.getAttribute(g.texCoord, 2) : null;

        // An osg array may be BIND_OVERALL (one element shared by all vertices).
        normal = this.perVertex(normal, vertexCount);
        color = this.perVertex(color, vertexCount);
        texCoord = this.perVertex(texCoord, vertexCount);

        for (const [topology, list] of groups) {
            if (topology === 'mode' || !list.length) continue;
            const geometry = new THREE.BufferGeometry();
            geometry.setAttribute('position', position);
            if (normal) geometry.setAttribute('normal', normal);
            if (texCoord) geometry.setAttribute('uv', texCoord);
            if (color) geometry.setAttribute('color', color);
            geometry.setIndex(vertexCount > 65535 ? new THREE.Uint32BufferAttribute(list, 1)
                                                  : new THREE.Uint16BufferAttribute(list, 1));

            let object;
            if (topology === 'lines') {
                object = new THREE.LineSegments(geometry, new THREE.LineBasicMaterial({ color: 0xffffff }));
            } else if (topology === 'points') {
                object = new THREE.Points(geometry, new THREE.PointsMaterial({ size: 2, color: 0xffffff }));
            } else {
                // A geometry without a normal array has nothing to light. osg/VSG
                // render those with the ambient term alone, i.e. the texture at
                // its own brightness (photogrammetry bakes lighting into the
                // texture already), so use an unlit material. Computing normals
                // here instead would produce NaN for degenerate triangles and
                // black patches.
                const common = {
                    color: params.diffuse,
                    map: texture,
                    vertexColors: !!color,
                    side: THREE.DoubleSide   // osg assets are not winding-consistent
                };
                const material = normal
                    ? new THREE.MeshPhongMaterial(Object.assign({
                        emissive: params.emissive,
                        specular: params.specular,
                        shininess: params.shininess
                    }, common))
                    : new THREE.MeshBasicMaterial(common);
                object = new THREE.Mesh(geometry, material);
            }
            g.node.add(object);
        }
    }

    perVertex(attr, n) {
        if (!attr) return null;
        if (attr.count === n) return attr;
        if (attr.count !== 1 || n === 0) return null;
        const comps = attr.itemSize;
        const data = new Float32Array(n * comps);
        for (let i = 0; i < n; ++i)
            for (let c = 0; c < comps; ++c) data[i * comps + c] = attr.array[c];
        return new THREE.Float32BufferAttribute(data, comps);
    }

    appendPrimitive(p, groups) {
        const topology = topologyOf(p.mode);
        if (!topology) return;
        let out = groups.get(topology);
        if (!out) { out = []; groups.set(topology, out); }

        if (p.lengths && p.lengths.length) {
            // DrawArrayLengths: each length restarts the primitive, so the runs
            // must be expanded independently.
            let offset = p.first;
            for (let k = 0; k < p.lengths.length; ++k) {
                const n = p.lengths[k];
                const src = [];
                for (let i = 0; i < n; ++i) src.push(offset + i);
                expand(p.mode, src, out);
                offset += n;
            }
        } else if (p.indices && p.indices.length) {
            expand(p.mode, p.indices, out);
        } else {
            const src = [];
            for (let i = 0; i < p.count; ++i) src.push(p.first + i);
            expand(p.mode, src, out);
        }
    }

    buildLod(lod) {
        if (!lod.node) return;
        const fallback = new THREE.Group();
        fallback.name = 'inMemory';
        for (const c of lod.children) fallback.add(c);
        lod.node.add(fallback);
        lod.node.userData.fallback = fallback;

        // First non-empty child name is the external (paged) subgraph.
        let external = '';
        const keys = Object.keys(lod.files).map(Number).sort((a, b) => a - b);
        for (const k of keys) { if (lod.files[k]) { external = lod.files[k]; break; } }

        if (lod.paged) {
            lod.node.userData.files = keys.map((k) => lod.files[k]).filter((f) => !!f);
            if (external) lod.node.userData.file = external;
            if (lod.center) lod.node.userData.center = lod.center;
            if (lod.radius) lod.node.userData.radius = lod.radius;

            // Ranges are osg's [min,max] eye-distance-in-radii: the child is
            // requested while closer than max. Turn that into a screen-height
            // ratio so the pager can decide purely from the camera.
            const rk = Object.keys(lod.ranges).map(Number).sort((a, b) => a - b);
            if (rk.length) lod.node.userData.range = lod.ranges[rk[0]][1];

            this.pagedNodes.push(lod.node);
        } else if (lod.children.length) {
            // A plain osg::LOD: three.js LOD would need screen-space metrics too,
            // so for this example the first child simply stays visible.
            lod.node.userData.lod = true;
        }
    }

    finalizePagedBounds(node) {
        const ud = node.userData;
        if (!ud.center || !ud.radius) {
            // Fall back to the in-memory child's bounding sphere (local space).
            const box = new THREE.Box3().setFromObject(ud.fallback || node);
            if (box.isEmpty()) { ud.center = new THREE.Vector3(); ud.radius = 1; return; }
            const sphere = box.getBoundingSphere(new THREE.Sphere());
            ud.center = sphere.center;
            ud.radius = sphere.radius;
        }
    }
}

// A shared 1x1 white image used as the placeholder for a texture that has not
// decoded yet (or whose file is missing), so the material samples white instead
// of an incomplete texture, which WebGL would render as black.
let _whitePixel = null;
function whitePixel() {
    if (_whitePixel) return _whitePixel;
    const canvas = document.createElement('canvas');
    canvas.width = canvas.height = 1;
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, 1, 1);
    _whitePixel = canvas;
    return _whitePixel;
}

// Texture.flipY is ignored for ImageBitmap sources (see three.js' Texture.flipY
// docs), so the vertical orientation has to be baked in when the bitmap is
// created. osg stores image rows bottom-up with v = 0 at the bottom of the
// picture, which is what imageOrientation 'flipY' produces from a normal JPEG.
// The raw blobs are kept so the UI can re-decode with the opposite orientation.
const bitmapTextures = new Set();
const imageTextures = new Set();

function decodeBitmap(tex, blob, flipY) {
    tex.userData.placeholder = true;
    return createImageBitmap(blob, { imageOrientation: flipY ? 'flipY' : 'none' })
        .then((bitmap) => {
            tex.userData.placeholder = false;
            tex.flipY = false;             // already baked into the bitmap
            tex.image = bitmap;
            tex.needsUpdate = true;
        })
        .catch(() => {
            // Older engines: fall back to an <img>, where flipY does apply.
            const url = URL.createObjectURL(blob);
            const img = new Image();
            img.onload = () => {
                tex.userData.placeholder = false;
                tex.flipY = flipY;
                tex.image = img;
                tex.needsUpdate = true;
            };
            img.src = url;
        });
}

// Re-orient every texture built so far (used by the "mirror textures V" option).
export function setTextureFlipY(flipY) {
    for (const rec of bitmapTextures) decodeBitmap(rec.tex, rec.blob, flipY);
    for (const tex of imageTextures) {
        tex.flipY = flipY;
        tex.needsUpdate = true;
    }
}

// Drop a texture that is being unloaded. Without this the tracking sets above
// (and the GPU allocations behind them) would outlive the tile that owns them.
export function releaseTexture(tex) {
    if (!tex) return;
    for (const rec of bitmapTextures) {
        if (rec.tex === tex) { bitmapTextures.delete(rec); break; }
    }
    imageTextures.delete(tex);
    if (tex.image && typeof tex.image.close === 'function') tex.image.close();
    tex.dispose();
}

// GL mode -> the list topology three.js draws directly.
function topologyOf(mode) {
    switch (mode) {
    case GL.POINTS: return 'points';
    case GL.LINES: case GL.LINE_LOOP: case GL.LINE_STRIP: return 'lines';
    case GL.TRIANGLES: case GL.TRIANGLE_STRIP: case GL.TRIANGLE_FAN:
    case GL.QUADS: case GL.QUAD_STRIP: case GL.POLYGON: return 'triangles';
    default: return 'triangles';
    }
}

// Expand a vertex/index sequence of `mode` into a plain triangle/line list
// (mirrors VsgSceneBuilder::expand).
function expand(mode, src, out) {
    const n = src.length;
    switch (mode) {
    case GL.TRIANGLE_STRIP:
        for (let i = 2; i < n; ++i) addTriangle(out, src[i - 2], src[i - 1], src[i], (i & 1) !== 0);
        break;
    case GL.TRIANGLE_FAN:
    case GL.POLYGON:
        for (let i = 2; i < n; ++i) addTriangle(out, src[0], src[i - 1], src[i], false);
        break;
    case GL.QUADS:
        for (let i = 0; i + 4 <= n; i += 4) {
            addTriangle(out, src[i], src[i + 1], src[i + 2], false);
            addTriangle(out, src[i], src[i + 2], src[i + 3], false);
        }
        break;
    case GL.QUAD_STRIP:
        for (let i = 0; i + 4 <= n; i += 2) {
            addTriangle(out, src[i], src[i + 1], src[i + 2], false);
            addTriangle(out, src[i + 1], src[i + 3], src[i + 2], false);
        }
        break;
    case GL.LINE_LOOP:
        for (let i = 0; i < n; ++i) { out.push(src[i], src[(i + 1) % n]); }
        break;
    case GL.LINE_STRIP:
        for (let i = 0; i + 1 < n; ++i) { out.push(src[i], src[i + 1]); }
        break;
    default:
        for (let i = 0; i < n; ++i) out.push(src[i]);
        break;
    }
}

function addTriangle(out, a, b, c, swap) {
    out.push(a, swap ? c : b, swap ? b : c);
}
