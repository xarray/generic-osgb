// reader.js -- load the WebAssembly reader and expose it as a simple async API.
//
// dist/generic_osg_wasm.js is built by CMake under Emscripten (see the
// ENABLE_EMSCRIPTEN target in CMakeLists.txt) and wraps the very same
// generic_wrappers reader used by the native examples.

import createGenericOSG from '../dist/generic_osg_wasm.js';

export class Reader {
    constructor(module) {
        this.module = module;
    }

    static async create() {
        return new Reader(await createGenericOSG());
    }

    // bytes: Uint8Array -> the JS-friendly SceneIR (see wrappers_wasm.cpp)
    read(bytes) {
        return this.module.readOSG(bytes);
    }

    async readUrl(url) {
        const response = await fetch(url);
        if (!response.ok)
            throw new Error(`${response.status} ${response.statusText} for ${url}`);
        const buffer = await response.arrayBuffer();
        return this.read(new Uint8Array(buffer));
    }
}
