// metadata.js -- read a ContextCapture/Smart3D metadata.xml and build the local
// frame the .osgb file lives in.
//
// The sample database declares:
//
//   <SRS>ENU:48.21221,124.61605</SRS>
//   <SRSOrigin>0,0,0</SRSOrigin>
//
// i.e. the tile coordinates are metres in an East-North-Up frame whose origin
// is (latitude, longitude) on the WGS84 ellipsoid. Cesium's
// `Transforms.eastNorthUpToFixedFrame` produces exactly that frame, so the same
// matrix converts file coordinates to ECEF and ECEF back to the frame the
// three.js overlay renders in.

export async function loadModelMetadata(url) {
    const response = await fetch(url);
    if (!response.ok)
        throw new Error(`${response.status} ${response.statusText} for ${url}`);

    const text = await response.text();
    const doc = new DOMParser().parseFromString(text, 'text/xml');
    if (doc.querySelector('parsererror'))
        throw new Error(`invalid metadata XML: ${url}`);

    const srs = (doc.querySelector('SRS') && doc.querySelector('SRS').textContent || '').trim();
    const originText = (doc.querySelector('SRSOrigin') && doc.querySelector('SRSOrigin').textContent || '0,0,0').trim();

    return Object.assign({ srs, origin: parseVec3(originText) }, parseSrs(srs));
}

function parseSrs(srs) {
    // ContextCapture writes "ENU:latitude,longitude".
    const enu = /^ENU:\s*([-+\d.eE]+)\s*,\s*([-+\d.eE]+)\s*$/.exec(srs);
    if (enu) {
        return {
            kind: 'ENU',
            latitude: parseFloat(enu[1]),
            longitude: parseFloat(enu[2])
        };
    }

    const epsg = /^EPSG:\s*(\d+)\s*$/i.exec(srs);
    if (epsg) return { kind: 'EPSG', code: parseInt(epsg[1], 10) };

    return { kind: 'UNKNOWN' };
}

function parseVec3(text) {
    const parts = text.split(',').map((s) => parseFloat(s));
    return {
        x: Number.isFinite(parts[0]) ? parts[0] : 0,
        y: Number.isFinite(parts[1]) ? parts[1] : 0,
        z: Number.isFinite(parts[2]) ? parts[2] : 0
    };
}

// Returns the matrices that convert between the file's ENU frame and ECEF.
// `originHeight` is the ellipsoidal height (metres) of the ENU origin, useful
// when the metadata carries no vertical datum and the site would otherwise
// hover above the Cesium ellipsoid.
export function createEnuFrame(meta, originHeight = 0) {
    if (meta.kind !== 'ENU') {
        throw new Error(`unsupported SRS "${meta.srs}": this example expects ENU:latitude,longitude`);
    }

    const base = Cesium.Cartesian3.fromDegrees(meta.longitude, meta.latitude, originHeight);
    const enuToEcef = Cesium.Transforms.eastNorthUpToFixedFrame(
        base, Cesium.Ellipsoid.WGS84, new Cesium.Matrix4());

    // SRSOrigin is an offset expressed in the ENU frame; applying it as a
    // translation keeps the tile coordinates untouched.
    if (meta.origin.x || meta.origin.y || meta.origin.z) {
        Cesium.Matrix4.multiplyByTranslation(
            enuToEcef, new Cesium.Cartesian3(meta.origin.x, meta.origin.y, meta.origin.z), enuToEcef);
    }

    const originECEF = Cesium.Matrix4.multiplyByPoint(
        enuToEcef, Cesium.Cartesian3.ZERO, new Cesium.Cartesian3());
    const ecefToEnu = Cesium.Matrix4.inverse(enuToEcef, new Cesium.Matrix4());

    return { originECEF, enuToEcef, ecefToEnu, meta };
}
