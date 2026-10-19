// wrappers_wasm: the generic_wrappers reader compiled to WebAssembly and
// exposed to JavaScript through embind.
//
// The three.js example (examples/threejs/) fetches an .osgb/.osgt file, hands
// the bytes to readOSG(), rebuilds the scene with three.js and pages the
// PagedLOD children itself. Only the consumer differs from examples/null and
// examples/vsg -- all of them go through the same neutral SceneIR.
//
// readOSG() returns a plain JS value so the adapter stays in JavaScript:
//
//   {
//     objects: [ { className, id, parent,
//                  im?:  { decision, fileName, origin, width, height, depth,
//                          internalFormat, pixelFormat, dataType, pixels },
//                  p?:   { type, mode, count, first, indexBytes, indices, lengths },
//                  vec?: { token, components, data } } ],
//     events:  [ { type, object, name?, args? } ]
//   }
//
// `type` is 0 for CREATE_OBJECT and 1 for APPLY_COMMAND (see IREvent). Object
// references in command arguments are pre-resolved to an object index, so the
// adapter never has to reproduce SceneIR's key/lookup scheme.

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <sstream>
#include <string>
#include <vector>

#include "SceneIR.h"

#ifdef OSGVERSE_GENERIC_USE_STATIC_ACTIVATION
#include "GenericStaticWrappers.h"   // keep every wrapper registrar in a plain static link
#endif

using emscripten::val;
using namespace osgVerse;

namespace
{
    val copyU8(const std::vector<unsigned char>& v)
    {
        if (v.empty()) return val::null();
        val view(emscripten::typed_memory_view(v.size(), const_cast<unsigned char*>(v.data())));
        return val::global("Uint8Array").new_(view);
    }

    val copyF64(const std::vector<double>& v)
    {
        if (v.empty()) return val::null();
        val view(emscripten::typed_memory_view(v.size(), const_cast<double*>(v.data())));
        return val::global("Float64Array").new_(view);
    }

    val copyU32(const std::vector<unsigned int>& v)
    {
        if (v.empty()) return val::null();
        val view(emscripten::typed_memory_view(v.size(), const_cast<unsigned int*>(v.data())));
        return val::global("Uint32Array").new_(view);
    }

    val argToJS(const SceneIR& ir, const IRArg& a)
    {
        val o = val::object();
        o.set("k", (int)a.kind);
        switch (a.kind)
        {
        case IRArg::BOOL:   o.set("n", a.b ? 1.0 : 0.0); break;
        case IRArg::INT:    o.set("n", (double)a.i); break;
        case IRArg::UINT:   o.set("n", (double)a.u); break;
        case IRArg::DOUBLE: o.set("n", a.d); break;
        case IRArg::STRING: o.set("s", a.s); break;
        case IRArg::OBJECT:
            o.set("t", a.objType);
            o.set("o", ir.resolve(a.objType, a.objId));
            break;
        default:
        {
            // MATH kinds: VEC2/3/4, QUAT, PLANE, MATRIX, BBOX, BSPHERE
            val v = val::array();
            for (int i = 0; i < a.n && i < 16; ++i) v.call<void>("push", a.v[i]);
            o.set("v", v);
            break;
        }
        }
        return o;
    }

    val eventToJS(const SceneIR& ir, const IREvent& e)
    {
        val o = val::object();
        o.set("type", (int)e.type);
        o.set("object", e.objectIndex);
        if (e.type == IREvent::APPLY_COMMAND)
        {
            o.set("name", e.cmd.name);
            val args = val::array();
            for (size_t i = 0; i < e.cmd.args.size(); ++i)
                args.call<void>("push", argToJS(ir, e.cmd.args[i]));
            o.set("args", args);
        }
        return o;
    }

    val objectToJS(const IRObject& obj)
    {
        val o = val::object();
        o.set("className", obj.className);
        o.set("id", obj.id);
        o.set("parent", obj.parent);

        // The recorder fills the data fields directly (arrayData / payload tags
        // are only used by other IR consumers), so key off the fields that are
        // actually present.
        const bool isPrimitive = obj.className.find("DrawArray") != std::string::npos ||
                                 obj.className.find("DrawElements") != std::string::npos ||
                                 obj.className == "osg::PrimitiveSet";
        if (isPrimitive)
        {
            val p = val::object();
            p.set("type", obj.primType);
            p.set("mode", obj.primMode);
            p.set("count", (double)obj.primCount);
            p.set("first", (double)obj.primFirst);
            p.set("indexBytes", obj.primIndexBytes);
            p.set("indices", copyU32(obj.primIndices));
            p.set("lengths", copyU32(obj.primLengths));
            o.set("p", p);
        }

        if (!obj.fileName.empty() || !obj.pixelData.empty() || obj.width > 0)
        {
            val im = val::object();
            im.set("decision", obj.imageDecision);
            im.set("fileName", obj.fileName);
            im.set("origin", obj.origin);
            im.set("width", obj.width);
            im.set("height", obj.height);
            im.set("depth", obj.depth);
            im.set("internalFormat", obj.internalFormat);
            im.set("pixelFormat", obj.pixelFormat);
            im.set("dataType", obj.dataType);
            im.set("pixels", copyU8(obj.pixelData));
            o.set("im", im);
        }

        if (!obj.vectorData.empty())
        {
            val v = val::object();
            v.set("token", obj.vectorToken);
            v.set("components", obj.vectorComponents);
            v.set("data", copyF64(obj.vectorData));
            o.set("vec", v);
        }
        return o;
    }
}

// Read .osgt/.osgb bytes (a Uint8Array) into the JS-friendly IR above.
// Returns null when the stream cannot be parsed.
val readOSG(val data)
{
    const std::vector<unsigned char> bytes =
        emscripten::convertJSArrayToNumberVector<unsigned char>(data);

    std::string buffer;
    buffer.assign(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::istringstream in(buffer, std::ios::in | std::ios::binary);

    SceneIR ir;
    if (!readSceneIR(in, ir, false)) return val::null();

    val result = val::object();
    val objects = val::array();
    for (size_t i = 0; i < ir.objects.size(); ++i)
        objects.call<void>("push", objectToJS(ir.objects[i]));
    val events = val::array();
    for (size_t i = 0; i < ir.events.size(); ++i)
        events.call<void>("push", eventToJS(ir, ir.events[i]));

    result.set("objects", objects);
    result.set("events", events);
    return result;
}

EMSCRIPTEN_BINDINGS(generic_osg_wasm)
{
    emscripten::function("readOSG", &readOSG);
}
