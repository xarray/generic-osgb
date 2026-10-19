// main.js -- the three.js / WebGL front end.
//
//   fetch .osgb/.osgt  ->  WASM reader  ->  SceneIR  ->  three.js scene graph
//                                                        + lightweight pager
//
// The root file is read once; PagedLOD children are streamed in by
// js/databasePager.js as the camera moves closer.

import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { Reader } from './reader.js';
import { buildScene, setTextureFlipY } from './sceneBuilder.js';
import { DatabasePager } from './databasePager.js';

const params = new URLSearchParams(location.search);
const MODEL_URL = params.get('model') || '/data/Tile_1/Tile_1.osgb';

let renderer, scene, camera, controls, pager, reader, pagerOptions;
let modelRoot = null;
let sceneBuilder = null;
let lastTime = performance.now();
let currentFlipY = true;

init().catch((err) => fail(err));

async function init() {
    const canvas = document.getElementById('view');
    renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.setSize(window.innerWidth, window.innerHeight);

    scene = new THREE.Scene();
    scene.background = new THREE.Color(0x101822);

    camera = new THREE.PerspectiveCamera(55, window.innerWidth / window.innerHeight, 0.1, 1e6);
    camera.position.set(0, 0, 10);

    // osg assets are Z-up; rotate the whole model into three's Y-up world.
    modelRoot = new THREE.Group();
    modelRoot.rotation.x = -Math.PI / 2;
    scene.add(modelRoot);

    // osg assets usually rely on a headlight, and photogrammetry textures
    // already contain their own baked lighting, so the ambient term dominates
    // and the directionals only add a little shape. The key light must sit on
    // +Y (the model is rotated into three's Y-up world above), otherwise the
    // terrain is lit from underneath and renders almost black.
    scene.add(new THREE.AmbientLight(0xffffff, 2.5));
    const key = new THREE.DirectionalLight(0xffffff, 1.0);
    key.position.set(0.6, 1.0, 0.7);
    scene.add(key);
    const fill = new THREE.DirectionalLight(0xffffff, 0.3);
    fill.position.set(-0.8, 0.5, -0.6);
    scene.add(fill);

    controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.12;

    window.addEventListener('resize', onResize);

    setStatus(`loading ${MODEL_URL} ...`);
    reader = await Reader.create();

    const ir = await reader.readUrl(MODEL_URL);
    if (!ir) throw new Error(`failed to read ${MODEL_URL}`);

    const builder = await buildScene(ir, { baseUrl: MODEL_URL, flipY: currentFlipY });
    sceneBuilder = builder;
    if (!builder.root) throw new Error('the file produced an empty scene');
    modelRoot.add(builder.root);

    pagerOptions = { baseUrl: MODEL_URL, flipY: currentFlipY };
    pager = new DatabasePager(reader, {
        ratio: parseFloat(document.getElementById('threshold').value),
        useOsgRange: document.getElementById('useOsgRange').checked,
        maxConcurrent: 4,
        maxPerFrame: 2,
        builderOptions: pagerOptions
    });
    pager.register(builder.root);

    frameCamera();

    document.getElementById('model').textContent = MODEL_URL;
    setStatus('');
    animate();
}

// The scene center is generally not at the origin: osg files carry their own
// coordinates. Prefer the bounding sphere the reader found (PagedLOD's
// center/radius, or a node's initial bound) and fall back to the geometry.
function frameCamera() {
    modelRoot.updateMatrixWorld(true);

    let center = null, radius = 0;
    if (sceneBuilder && sceneBuilder.bounds) {
        const b = sceneBuilder.bounds;   // file coordinate system
        center = b.center.clone().applyMatrix4(modelRoot.matrixWorld);
        radius = b.radius;
    }
    if (!center || !(radius > 0)) {
        const box = new THREE.Box3().setFromObject(modelRoot);
        if (box.isEmpty()) {
            camera.position.set(0, 5, 10);
            controls.target.set(0, 0, 0);
            controls.update();
            return;
        }
        const sphere = box.getBoundingSphere(new THREE.Sphere());
        center = sphere.center.clone();
        radius = sphere.radius || 1;
    }

    controls.target.copy(center);
    camera.position.copy(center).add(new THREE.Vector3(0.9, 0.7, 1.0).normalize().multiplyScalar(radius * 1.8));
    camera.near = Math.max(radius / 1000, 0.01);
    camera.far = radius * 200 + 1000;
    camera.updateProjectionMatrix();
    controls.update();
}

function animate() {
    requestAnimationFrame(animate);
    const now = performance.now();
    const dt = (now - lastTime) / 1000;
    lastTime = now;
    controls.update();

    modelRoot.updateMatrixWorld(true);
    pager.update(camera);

    renderer.render(scene, camera);
    updateStats(dt);
}

let frames = 0, acc = 0;
function updateStats(dt) {
    acc += dt; ++frames;
    if (acc < 0.25) return;
    const s = pager.stats;
    const info = sceneInfo(modelRoot);
    document.getElementById('stats').textContent =
        `fps ${(frames / acc).toFixed(0)}  |  meshes ${info.meshes}  tris ${info.tris}  ` +
        `|  paged nodes ${s.registered}  |  loading ${s.pending}  |  queued ${s.queued}  ` +
        `|  resident ${s.loaded}  |  unloaded ${s.unloaded}  ` +
        `|  cache ${s.cacheSize} (${(s.cacheBytes / 1048576).toFixed(1)} MB, hits ${s.cacheHits})` +
        (s.failed ? `  |  failed ${s.failed}` : '');
    document.getElementById('debug').textContent = textureInfo(modelRoot);
    acc = 0; frames = 0;
}

function textureInfo(root) {
    let meshes = 0, mapped = 0, ready = 0, size = '-', mean = '-';
    root.traverse((n) => {
        if (!n.isMesh) return;
        ++meshes;
        const map = n.material && n.material.map;
        if (!map) return;
        ++mapped;
        const pending = map.userData && map.userData.placeholder;
        if (map.image && !pending) {
            ++ready;
            if (size === '-') {
                size = `${map.image.width}x${map.image.height}`;
                const m = probeTexture(map.image);
                mean = m ? `mean rgb(${m.join(',')})` : 'mean n/a';
            }
        }
    });
    return `textures ${ready}/${mapped} decoded (${meshes} meshes), first ${size}` +
           `, ${mean}, flipY ${currentFlipY}`;
}

// Average colour of a decoded texture image -- a quick way to tell whether the
// embedded JPEG actually decoded to something, or to black.
const _probeCache = new WeakMap();
function probeTexture(image) {
    if (_probeCache.has(image)) return _probeCache.get(image);
    let result = null;
    try {
        const w = Math.min(image.width, 64), h = Math.min(image.height, 64);
        const canvas = document.createElement('canvas');
        canvas.width = w; canvas.height = h;
        const ctx = canvas.getContext('2d');
        ctx.drawImage(image, 0, 0, w, h);
        const data = ctx.getImageData(0, 0, w, h).data;
        let r = 0, g = 0, b = 0;
        const n = data.length / 4;
        for (let i = 0; i < data.length; i += 4) { r += data[i]; g += data[i + 1]; b += data[i + 2]; }
        result = [Math.round(r / n), Math.round(g / n), Math.round(b / n)];
    } catch (err) {
        result = null;
    }
    _probeCache.set(image, result);
    return result;
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

function onResize() {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
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

// The slider / checkbox can be adjusted while running: ratio 0 makes every
// paged child resident, and clearing the checkbox uses the manual ratio even
// for files that carry osg ranges.
document.getElementById('threshold').addEventListener('input', (e) => {
    if (pager) pager.ratio = parseFloat(e.target.value);
    document.getElementById('thresholdValue').textContent = parseFloat(e.target.value).toFixed(2);
});
document.getElementById('useOsgRange').addEventListener('change', (e) => {
    if (pager) pager.useOsgRange = e.target.checked;
});

// Dolly the camera along the view direction -- the same motion as the mouse
// wheel, and the way to fly towards the model and trigger the pager.
function dolly(factor) {
    const offset = camera.position.clone().sub(controls.target).multiplyScalar(factor);
    camera.position.copy(controls.target).add(offset);
    controls.update();
}
document.getElementById('zoomIn').addEventListener('click', () => dolly(0.65));
document.getElementById('zoomOut').addEventListener('click', () => dolly(1 / 0.65));

// osg stores image rows bottom-up and puts v = 0 at the bottom of the picture.
// Note three.js ignores Texture.flipY for ImageBitmap sources, so the corner is
// baked in at decode time instead; this re-decodes every texture built so far.
document.getElementById('flipTexture').addEventListener('change', (e) => {
    currentFlipY = !e.target.checked;
    if (pagerOptions) pagerOptions.flipY = currentFlipY;
    setTextureFlipY(currentFlipY);
});
