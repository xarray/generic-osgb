#ifndef OSGVERSE_GENERIC_WRAPPERS_SCENE_IR_H
#define OSGVERSE_GENERIC_WRAPPERS_SCENE_IR_H

// SceneIR: the engine-neutral intermediate representation produced by the
// generic_wrappers reader and consumed by per-engine adapters (SceneBuilder).
//
// It is deliberately free of any osg (or other engine) dependency: it only
// stores plain values, so the same IR can be replayed into OpenSceneGraph,
// VSG, a Web engine, Godot, Unity, Unreal, etc.
//
// Reading a file produces a linear event stream (create-object / command),
// which keeps the original depth-first read order and therefore mirrors how
// osg itself reconstructs objects.

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace osgVerse
{

// ---------------------------------------------------------------------------
// A single typed command argument. Covers every argument shape emitted by the
// generic_osg wrapper read functions and by the serializer property channel.
// ---------------------------------------------------------------------------
struct IRArg
{
    enum Kind
    {
        NONE = 0, BOOL, INT, UINT, DOUBLE, STRING,        // scalars (STRING also used for enum tokens)
        OBJECT,                                            // reference to another IR object (type + id)
        VEC2, VEC3, VEC4, QUAT, PLANE, MATRIX, BBOX, BSPHERE
    };

    Kind kind;
    bool b;
    long long i;
    unsigned long long u;
    double d;
    std::string s;          // STRING value / enum token
    std::string objType;    // OBJECT reference: class name
    std::string objId;      // OBJECT reference: unique id
    double v[16];           // math components (VEC2/3/4, QUAT, PLANE, MATRIX, BBOX, BSPHERE)
    int n;                  // number of valid math components

    IRArg()
        : kind(NONE), b(false), i(0), u(0), d(0), n(0)
    { for (int k = 0; k < 16; ++k) v[k] = 0.0; }

    static IRArg makeBool(bool value) { IRArg a; a.kind = BOOL; a.b = value; return a; }
    static IRArg makeInt(long long value) { IRArg a; a.kind = INT; a.i = value; return a; }
    static IRArg makeUInt(unsigned long long value) { IRArg a; a.kind = UINT; a.u = value; return a; }
    static IRArg makeDouble(double value) { IRArg a; a.kind = DOUBLE; a.d = value; return a; }
    static IRArg makeString(const std::string& value) { IRArg a; a.kind = STRING; a.s = value; return a; }
    static IRArg makeObject(const std::string& type, const std::string& id)
    { IRArg a; a.kind = OBJECT; a.objType = type; a.objId = id; return a; }
};

// ---------------------------------------------------------------------------
// A recorded command: a named method call with arguments, applied to an object.
// Both explicit `ud.add("method", args...)` calls and implicit serializer
// property assignments ("PropertyName", value) are represented this way, so an
// adapter only needs one dispatch path.
// ---------------------------------------------------------------------------
struct IRCommand
{
    std::string name;
    std::vector<IRArg> args;
};

// ---------------------------------------------------------------------------
// A recorded object. `payload` distinguishes the special data-bearing records
// produced by readArray()/readImage()/readPrimitiveSet() from regular wrapper
// objects.
// ---------------------------------------------------------------------------
struct IRObject
{
    enum Payload { NONE = 0, ARRAY, IMAGE, PRIMITIVE };

    std::string className;
    std::string id;
    int parent;              // parent object index, or -1
    long long offset;        // file offset where the object started
    Payload payload;

    // ARRAY payload
    int arrayType;
    int arrayNumComponents;
    int arrayComponentBytes;
    unsigned int arrayNumElements;
    std::vector<unsigned char> arrayData;

    // IMAGE payload
    std::string fileName;    // external file reference (decision == USE_FILE)
    int origin, width, height, depth;
    int internalFormat, pixelFormat, dataType, packing, allocationMode;
    std::vector<unsigned char> pixelData;

    // PRIMITIVE payload
    int primType;            // osg PrimitiveSet type id (ID_DRAWARRAYS, ID_DRAWELEMENTS_* ...)
    int primMode;            // GL mode (GL_TRIANGLES ...)
    unsigned int primCount;  // DrawArrays: count; DrawElements: number of indices;
                             // DrawArrayLengths: total vertex count (sum of lengths)
    unsigned int primFirst;  // DrawArrays: first index
    int primIndexBytes;      // DrawElements: index element size in bytes (0 for DrawArrays)
    std::vector<unsigned int> primIndices;
    // DrawArrayLengths only: each entry is an independent run of the mode's
    // primitive (a GL_TRIANGLE_STRIP run restarts at every length), so the runs
    // must be expanded separately instead of as one long primitive.
    std::vector<unsigned int> primLengths;

    // VectorSerializer payload (an osg::Array's or DrawElements' "vector" data)
    std::string vectorToken;              // element type token, e.g. "RW_VEC3F"
    int vectorComponents;                 // components per element
    std::vector<double> vectorData;       // flattened element data

    IRObject()
        : parent(-1), offset(0), payload(NONE),
          arrayType(0), arrayNumComponents(0), arrayComponentBytes(0), arrayNumElements(0),
          origin(0), width(0), height(0), depth(0),
          internalFormat(0), pixelFormat(0), dataType(0), packing(0), allocationMode(0),
          primType(0), primMode(0), primCount(0), primFirst(0), primIndexBytes(0),
          vectorComponents(0)
    {}
};

// ---------------------------------------------------------------------------
// A linear event: either "create object #index" or "apply command to #index".
// ---------------------------------------------------------------------------
struct IREvent
{
    enum Type { CREATE_OBJECT = 0, APPLY_COMMAND } type;
    int objectIndex;
    IRCommand cmd;
};

// ---------------------------------------------------------------------------
// The IR container.
// ---------------------------------------------------------------------------
class SceneIR
{
public:
    std::vector<IRObject> objects;
    std::vector<IREvent> events;

    // Create a regular wrapper object; returns its index. `parent` is the IR
    // index of the enclosing object (or -1).
    int createObject(const std::string& className, const std::string& id,
                     int parent, long long offset);

    // Create a data-bearing object (array / image / primitive) with a key that
    // makes it addressable as an OBJECT reference target.
    int createPayload(const IRObject& obj);

    // Append a command targeting an object index.
    void emitCommand(int targetIndex, const std::string& name, const std::vector<IRArg>& args);

    // Resolve an OBJECT reference (class name + id) to an object index, or -1.
    int resolve(const std::string& className, const std::string& id) const;

    // Key used to store/lookup object references (namespace-separated so that
    // numeric ids from the array/image/object maps cannot collide).
    static std::string refKey(const std::string& className, const std::string& id);

    void clear();

protected:
    std::map<std::string, int> _keyToIndex;
};

// ---------------------------------------------------------------------------
// Engine adapter interface. Implement onObject/onCommand to rebuild the scene
// in a concrete engine; buildScene() replays the IR in read order.
// ---------------------------------------------------------------------------
struct ISceneBuilder
{
    virtual ~ISceneBuilder() {}
    virtual void onObject(const IRObject& object, int index) = 0;
    virtual void onCommand(const IRObject& object, int index, const IRCommand& command) = 0;
    virtual void finish() {}
};

// Replay the IR into a builder, in the order the objects/commands were read.
void buildScene(const SceneIR& ir, ISceneBuilder& builder);

// Read an .osgt/.osgb file and fill the neutral IR. Defined in the generic
// layer (which owns the reader); declared here so an engine TU can call it
// without pulling in the mini-osg math layer (which would clash with real osg).
bool readSceneIR(const std::string& filename, SceneIR& ir, bool forceAscii = false);

}

#endif
