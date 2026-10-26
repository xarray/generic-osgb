// main.js -- Cesium owns the globe and the camera; the OSGB tiles are rebuilt
// by the same SceneIR -> three.js adapter used by examples/threejs and drawn on
// a transparent canvas on top of the Cesium scene.
//
// Why the overlay: Cesium's dynamic scheduler is bound to 3D Tiles, glTF models
// and terrain. An OSGB PagedLOD tree is none of those, so Cesium cannot page it
// directly. Instead the existing JS DatabasePager (js/databasePager.js in the
// three.js example) is reused unchanged; Cesium only supplies the georeferenced
// globe and the camera.
//
// Coordinates: the metadata's ENU frame is used as the three.js world (X east,
// Y north, Z up), so the tile geometry is used as-is. Every frame the Cesium
// camera is transformed from ECEF into that same ENU frame; no per-vertex ECEF
// conversion is needed, and local coordinates stay small (no float jitter at
// 6.3e6 m from the earth centre).

import * as THREE from 'three';
import { Reader } from '../../threejs/js/reader.js';
import { buildScene, setTextureFlipY } from '../../threejs/js/sceneBuilder.js';
import { DatabasePager } from '../../threejs/js/databasePager.js';
import { loadModelMetadata, createEnuFrame } from './metadata.js';

const params = new URLSearchParams(location.search);
const MODEL_URL = params.get('model') || '/data/Tile_1/Tile_1.osgb';
const METADATA_URL = params.get('metadata') || '/data/metadata.xml';
// Optional ellipsoidal height of the metadata's ENU origin (metres); Cesium's
// ellipsoid is the ground reference when no terrain is loaded.
const ORIGIN_HEIGHT = parseFloat(params.get('up') || '0') || 0;

let viewer, renderer, scene, camera, reader, pager, modelRoot, sceneBuilder, frame;
let pagerOptions = null;
let flipY = true;
let ready = false;
let imageryMode = 'none';
let groundOffset = 0;

// model bounds in the file's ENU frame, and their ECEF position
let modelCenter = new THREE.Vector3();
let modelRadius = 1;
let modelCenterECEF = null;

const _localPos = new Cesium.Cartesian3();
const _localDir = new Cesium.Cartesian3();
const _localUp = new Cesium.Cartesian3();
const _dir = new THREE.Vector3();
const _up = new THREE.Vector3();
const _right = new THREE.Vector3();
const _back = new THREE.Vector3();
const _basis = new THREE.Matrix4();
const _v0 = new THREE.Vector3();
const _v1 = new THREE.Vector3();
const _v2 = new THREE.Vector3();
const _ab = new THREE.Vector3();
const _ac = new THREE.Vector3();
const _cross = new THREE.Vector3();

init().catch((err) => fail(err));

async function init() {
    const meta = await loadModelMetadata(METADATA_URL);
    frame = createEnuFrame(meta, ORIGIN_HEIGHT);

    document.getElementById('model').textContent = MODEL_URL;
    document.getElementById('srs').textContent = `SRS ${meta.srs}`;

    setupCesium();
    setupThree();
    setupImagery();

    setStatus(`reading ${MODEL_URL} ...`);
    reader = await Reader.create();

    const ir = await reader.readUrl(MODEL_URL);
    if (!ir) throw new Error(`failed to read ${MODEL_URL}`);

    const builder = await buildScene(ir, { baseUrl: MODEL_URL, flipY });
    sceneBuilder = builder;
    if (!builder.root) throw new Error('the file produced an empty scene');
    modelRoot.add(builder.root);
    groundOffset = alignToGround();

    pagerOptions = { baseUrl: MODEL_URL, flipY };
    pager = new DatabasePager(reader, {
        ratio: 0.25,
        useOsgRange: document.getElementById('useOsgRange').checked,
        maxConcurrent: 4,
        maxPerFrame: 2,
        builderOptions: pagerOptions
    });
    pager.register(builder.root);

    computeModelBounds();
    frameView();

    setStatus('');
    ready = true;
    viewer.scene.postRender.addEventListener(onPostRender);
}

// ---------------------------------------------------------------------------
// Cesium: globe + camera only. No imagery is configured so the example works
// offline; the globe is drawn with a flat base colour.
// ---------------------------------------------------------------------------
function setupCesium() {
    viewer = new Cesium.Viewer('cesium', {
        baseLayer: false,
        animation: false,
        timeline: false,
        geocoder: false,
        homeButton: false,
        sceneModePicker: false,
        baseLayerPicker: false,
        navigationHelpButton: false,
        fullscreenButton: false,
        infoBox: false,
        selectionIndicator: false,
        shouldAnimate: false
    });

    const scene3d = viewer.scene;
    scene3d.backgroundColor = Cesium.Color.fromCssColorString('#05080c');
    scene3d.globe.baseColor = Cesium.Color.fromCssColorString('#12212e');
    scene3d.globe.showGroundAtmosphere = true;
    scene3d.skyAtmosphere.show = true;
    scene3d.screenSpaceCameraController.minimumZoomDistance = 5.0;
    scene3d.screenSpaceCameraController.maximumZoomDistance = 2.0e7;
}

// ---------------------------------------------------------------------------
// Base map. ArcGIS World Imagery by default (remote, no token). `?imagery=`
// selects `osm` or `none`. The example is online-only: CesiumJS itself is
// loaded from jsDelivr, so no bundled imagery is needed as a fallback.
// ---------------------------------------------------------------------------
async function setupImagery() {
    const mode = (params.get('imagery') || 'arcgis').toLowerCase();
    if (mode === 'none') return;

    try {
        if (mode === 'osm') {
            viewer.imageryLayers.addImageryProvider(new Cesium.OpenStreetMapImageryProvider({
                url: 'https://tile.openstreetmap.org/'
            }));
            imageryMode = 'osm';
            return;
        }

        const provider = await Cesium.ArcGisMapServerImageryProvider.fromUrl(
            'https://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer',
            { enablePickFeatures: false });
        if (viewer.isDestroyed()) return;
        viewer.imageryLayers.addImageryProvider(provider);
        imageryMode = 'arcgis';
    } catch (err) {
        console.warn('[cesium] imagery failed to load', err);
    }
}

// ---------------------------------------------------------------------------
// three.js: transparent overlay canvas, same builder/pager as examples/threejs.
// ---------------------------------------------------------------------------
function setupThree() {
    renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setClearColor(0x000000, 0);
    renderer.domElement.id = 'overlay';
    document.body.appendChild(renderer.domElement);

    scene = new THREE.Scene();
    camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 0.5, 50000);

    // ENU is Z-up: the model needs no rotation (unlike the Y-up three.js example).
    modelRoot = new THREE.Group();
    scene.add(modelRoot);

    scene.add(new THREE.AmbientLight(0xffffff, 2.2));
    const key = new THREE.DirectionalLight(0xffffff, 1.1);
    key.position.set(0.4, -0.6, 0.9);
    scene.add(key);
    const fill = new THREE.DirectionalLight(0xffffff, 0.35);
    fill.position.set(-0.7, 0.5, 0.4);
    scene.add(fill);

    window.addEventListener('resize', onResize);
}

function onResize() {
    renderer.setSize(window.innerWidth, window.innerHeight);
}

// The file's ENU z values are heights relative to the metadata origin, which
// puts this photogrammetry site ~269 m above the Cesium ellipsoid. Shift the
// whole model down so its ground surface sits on the ellipsoid (or on the
// `up=` height); the pager picks the shift up through node.matrixWorld.
//
// The lowest vertex is not the ground: this database contains a few degenerate
// triangles ~13 m below the real surface. The ground is instead estimated from
// the triangle-area distribution (the horizontal terrain dominates the area),
// taking the 5% area quantile so the bulk of the model stays above the ground.
function alignToGround() {
    modelRoot.position.set(0, 0, 0);
    modelRoot.updateMatrixWorld(true);

    let groundZ = estimateGroundZ(modelRoot);
    if (groundZ === null) {
        const box = new THREE.Box3().setFromObject(modelRoot);
        if (box.isEmpty() || !Number.isFinite(box.min.z)) {
            console.warn('[cesium] no geometry bounds, cannot align to ground');
            return 0;
        }
        groundZ = box.min.z;
    }

    const offset = -groundZ;
    modelRoot.position.z = offset;
    modelRoot.updateMatrixWorld(true);
    return offset;
}

function estimateGroundZ(root) {
    const BIN = 0.5;
    const bins = new Map();
    let total = 0;

    root.traverse((n) => {
        if (!n.isMesh || !n.geometry) return;
        const position = n.geometry.attributes.position;
        if (!position) return;

        const index = n.geometry.index;
        const count = index ? index.count : position.count;
        for (let i = 0; i + 2 < count; i += 3) {
            const a = index ? index.getX(i) : i;
            const b = index ? index.getX(i + 1) : i + 1;
            const c = index ? index.getX(i + 2) : i + 2;

            _v0.fromBufferAttribute(position, a).applyMatrix4(n.matrixWorld);
            _v1.fromBufferAttribute(position, b).applyMatrix4(n.matrixWorld);
            _v2.fromBufferAttribute(position, c).applyMatrix4(n.matrixWorld);

            _ab.subVectors(_v1, _v0);
            _ac.subVectors(_v2, _v0);
            const area = _cross.crossVectors(_ab, _ac).length() * 0.5;
            if (!(area > 1e-6)) continue;   // degenerate triangles

            const key = Math.round((_v0.z + _v1.z + _v2.z) / 3 / BIN);
            bins.set(key, (bins.get(key) || 0) + area);
            total += area;
        }
    });

    if (!total) return null;

    const keys = Array.from(bins.keys()).sort((a, b) => a - b);
    const wanted = total * 0.05;
    let accumulated = 0;
    for (const key of keys) {
        accumulated += bins.get(key);
        if (accumulated >= wanted) return key * BIN;
    }
    return keys[keys.length - 1] * BIN;
}

// Put the Cesium camera above the model. The bounds are in the file's ENU frame
// after the ground shift and are converted to ECEF only for this camera setup.
function computeModelBounds() {
    modelRoot.updateMatrixWorld(true);

    let center = null, radius = 0;
    const box = new THREE.Box3().setFromObject(modelRoot);
    if (!box.isEmpty()) {
        const sphere = box.getBoundingSphere(new THREE.Sphere());
        center = sphere.center;
        radius = sphere.radius;
    } else if (sceneBuilder && sceneBuilder.bounds && sceneBuilder.bounds.radius > 0) {
        center = sceneBuilder.bounds.center.clone();
        center.z += groundOffset;
        radius = sceneBuilder.bounds.radius;
    }

    modelCenter.copy(center || new THREE.Vector3());
    modelRadius = Math.max(radius || 1, 0.01);
    modelCenterECEF = Cesium.Matrix4.multiplyByPoint(
        frame.enuToEcef,
        new Cesium.Cartesian3(modelCenter.x, modelCenter.y, modelCenter.z),
        new Cesium.Cartesian3());
}

function frameView() {
    setEnuView(modelRadius * 2.4, 25, -35);
}

// Pull back far enough to see the imagery and the curvature of the globe.
function globeView() {
    setEnuView(Math.max(modelRadius * 100, 8000), 20, -45);
}

// Place the Cesium camera from the dataset's ENU frame directly. This is the
// inverse of the per-frame camera sync below and avoids flight animations whose
// duration depends on the previous camera distance.
function setEnuView(range, headingDeg, pitchDeg) {
    if (!modelCenterECEF) return;

    const heading = Cesium.Math.toRadians(headingDeg);
    const elevation = Cesium.Math.toRadians(-pitchDeg);   // pitch < 0 looks down
    const local = new Cesium.Cartesian3(
        modelCenter.x + Math.sin(heading) * Math.cos(elevation) * range,
        modelCenter.y + Math.cos(heading) * Math.cos(elevation) * range,
        modelCenter.z + Math.sin(elevation) * range);
    const position = Cesium.Matrix4.multiplyByPoint(frame.enuToEcef, local, new Cesium.Cartesian3());

    const direction = Cesium.Cartesian3.normalize(
        Cesium.Cartesian3.subtract(modelCenterECEF, position, new Cesium.Cartesian3()),
        new Cesium.Cartesian3());
    const up4 = Cesium.Matrix4.getColumn(frame.enuToEcef, 2, new Cesium.Cartesian4());
    const up = new Cesium.Cartesian3(up4.x, up4.y, up4.z);

    viewer.camera.setView({ destination: position, orientation: { direction, up } });
}

// ---------------------------------------------------------------------------
// Per-frame sync: Cesium camera (ECEF) -> three camera (ENU).
// ---------------------------------------------------------------------------
function onPostRender() {
    if (!ready) return;

    syncCameraFromCesium();
    modelRoot.updateMatrixWorld(true);
    pager.update(camera);
    renderer.render(scene, camera);
    updateHud();
}

function syncCameraFromCesium() {
    const cam = viewer.camera;
    if (!cam.frustum || typeof cam.frustum.fovy !== 'number') return;   // 2D/Columbus modes

    Cesium.Matrix4.multiplyByPoint(frame.ecefToEnu, cam.positionWC, _localPos);
    Cesium.Matrix4.multiplyByPointAsVector(frame.ecefToEnu, cam.directionWC, _localDir);
    Cesium.Matrix4.multiplyByPointAsVector(frame.ecefToEnu, cam.upWC, _localUp);

    camera.position.set(_localPos.x, _localPos.y, _localPos.z);
    _dir.set(_localDir.x, _localDir.y, _localDir.z).normalize();
    _up.set(_localUp.x, _localUp.y, _localUp.z).normalize();

    // three's camera looks down -Z; Cesium's basis is (right, up, direction).
    _right.crossVectors(_dir, _up).normalize();
    _up.crossVectors(_right, _dir).normalize();
    _back.copy(_dir).negate();
    _basis.makeBasis(_right, _up, _back);
    camera.quaternion.setFromRotationMatrix(_basis);

    camera.aspect = window.innerWidth / Math.max(window.innerHeight, 1);
    camera.fov = Cesium.Math.toDegrees(cam.frustum.fovy);
    camera.near = Math.max(cam.frustum.near, 0.5);
    camera.far = Math.min(cam.frustum.far, 50000);
    camera.updateProjectionMatrix();
    camera.updateMatrixWorld(true);
    camera.matrixWorldInverse.copy(camera.matrixWorld).invert();
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------
let hudTime = performance.now();
let hudFrames = 0;

function updateHud() {
    const now = performance.now();
    ++hudFrames;
    if (now - hudTime < 500) return;

    const fps = hudFrames * 1000 / (now - hudTime);
    hudTime = now;
    hudFrames = 0;

    const s = pager.stats;
    const info = sceneInfo(modelRoot);
    document.getElementById('stats').textContent =
        `fps ${fps.toFixed(0)}  |  meshes ${info.meshes}  tris ${info.tris}  ` +
        `|  resident ${s.loaded}  |  loading ${s.pending}  |  queued ${s.queued}  ` +
        `|  cache ${s.cacheSize} (${(s.cacheBytes / 1048576).toFixed(1)} MB, hits ${s.cacheHits})`;

    const carto = modelCenterECEF ? Cesium.Cartographic.fromCartesian(modelCenterECEF) : null;
    const ecef = modelCenterECEF
        ? `${modelCenterECEF.x.toFixed(1)}, ${modelCenterECEF.y.toFixed(1)}, ${modelCenterECEF.z.toFixed(1)}`
        : '-';
    const geo = carto
        ? `lon ${Cesium.Math.toDegrees(carto.longitude).toFixed(6)}  lat ${Cesium.Math.toDegrees(carto.latitude).toFixed(6)}  h ${carto.height.toFixed(1)} m`
        : '';
    document.getElementById('debug').textContent =
        `center ECEF ${ecef}\n${geo}  |  ground offset ${groundOffset.toFixed(1)} m`;

    document.getElementById('srs').textContent =
        `SRS ${frame.meta.srs}  |  eye height ${viewer.camera.positionCartographic.height.toFixed(0)} m  ` +
        `|  basemap ${imageryMode}`;
}

function sceneInfo(root) {
    let meshes = 0, tris = 0;
    root.traverse((n) => {
        if (!n.isMesh || !n.geometry) return;
        ++meshes;
        const index = n.geometry.index;
        const position = n.geometry.attributes.position;
        tris += index ? index.count / 3 : (position ? position.count / 3 : 0);
    });
    return { meshes, tris: Math.round(tris) };
}

function setStatus(text) {
    const el = document.getElementById('status');
    el.textContent = text;
    el.style.display = text ? 'block' : 'none';
}

function fail(err) {
    console.error(err);
    setStatus(String(err && err.message ? err.message : err));
}

// ---------------------------------------------------------------------------
// controls
// ---------------------------------------------------------------------------
document.getElementById('useOsgRange').addEventListener('change', (e) => {
    if (pager) pager.useOsgRange = e.target.checked;
});

document.getElementById('resetView').addEventListener('click', () => frameView());
document.getElementById('globeView').addEventListener('click', () => globeView());

document.getElementById('flipTexture').addEventListener('change', (e) => {
    flipY = !e.target.checked;
    if (pagerOptions) pagerOptions.flipY = flipY;
    setTextureFlipY(flipY);
});
