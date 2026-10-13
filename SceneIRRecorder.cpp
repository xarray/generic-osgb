// SceneIRRecorder: the default recording SerializerVisitor.
//
// It walks the reader's serializer callbacks and fills a SceneIR (the optional
// neutral product), so an engine adapter can rebuild the scene in a separate
// translation unit without including the mini-osg math layer.

#include "GenericReader.h"
#include "GenericReserializer.h"
#include "SceneIR.h"

namespace osgVerse
{

namespace
{
    class IRRecorderVisitor : public SerializerVisitor
    {
    public:
        explicit IRRecorderVisitor(SceneIR& ir) : _ir(ir) { registerScalars(); }

        void beginObject(const std::string& className, const std::string& id) override
        {
            int parent = _stack.empty() ? -1 : _stack.back();
            int idx = _ir.createObject(className, id, parent, 0);
            _stack.push_back(idx);
        }
        void endObject() override
        {
            if (!_stack.empty()) _stack.pop_back();
        }
        void command(const std::string& name, const std::vector<IRArg>& args) override
        {
            if (current() >= 0) _ir.emitCommand(current(), name, args);
        }

        // ---- serializer callbacks ----
        void apply(BoolSerializer& s) override { emitProp(IRArg::makeBool(s._v0)); }
        void apply(StringSerializer& s) override { emitProp(IRArg::makeString(s._v0)); }
        void apply(EnumSerializer& s) override { emitProp(IRArg::makeString(s._v0)); }
        void apply(GLenumSerializer& s) override { emitProp(IRArg::makeUInt(s._v0)); }
        void apply(BitFlagSerializer& s) override { emitProp(IRArg::makeUInt(s._v0)); }
        void apply(ObjectSerializer& s) override
        { emitProp(IRArg::makeObject(s._readValue._type, s._readValue._id)); }
        void apply(ImageSerializer& s) override
        { emitProp(IRArg::makeObject(s._readValue._type, s._readValue._id)); }

        void apply(VectorSerializer& s) override
        {
            int cur = current(); if (cur < 0) return;
            IRObject& o = _ir.objects[cur];
            o.vectorToken = s._type;
            o.vectorComponents = s._numComponents;
            o.vectorData = s._values;

            std::vector<IRArg> args;
            _ir.emitCommand(cur, currentProperty().empty() ? "vector" : currentProperty(), args);
        }

        void apply(ListSerializer<ObjectTypeAndID>& s) override
        {
            int cur = current(); if (cur < 0) return;
            std::vector<IRArg> args;
            args.reserve(s._elements.size());
            for (size_t i = 0; i < s._elements.size(); ++i)
                args.push_back(IRArg::makeObject(s._elements[i]._type, s._elements[i]._id));
            _ir.emitCommand(cur, currentProperty(), args);
        }

        // ---- data-bearing objects from the legacy/compact readers ----
        void arrayData(int components, const std::vector<double>& values) override
        {
            int cur = current(); if (cur < 0) return;
            IRObject& o = _ir.objects[cur];
            o.vectorComponents = components;
            o.vectorData = values;
        }

        void primitiveData(int mode, unsigned int first, unsigned int count,
                           const std::vector<unsigned int>& indices,
                           const std::vector<unsigned int>& lengths) override
        {
            int cur = current(); if (cur < 0) return;
            IRObject& o = _ir.objects[cur];
            o.primMode = mode; o.primFirst = first; o.primCount = count;
            o.primIndices = indices;
            o.primLengths = lengths;
        }

        void imageData(const std::string& fileName, int origin,
                       int width, int height, int depth,
                       int internalFormat, int pixelFormat, int dataType,
                       int packing, int allocationMode,
                       const std::vector<unsigned char>& pixels) override
        {
            int cur = current(); if (cur < 0) return;
            IRObject& o = _ir.objects[cur];
            o.fileName = fileName; o.origin = origin;
            o.width = width; o.height = height; o.depth = depth;
            o.internalFormat = internalFormat; o.pixelFormat = pixelFormat; o.dataType = dataType;
            o.packing = packing; o.allocationMode = allocationMode;
            o.pixelData = pixels;
        }

    protected:
        int current() const { return _stack.empty() ? -1 : _stack.back(); }

        void emitProp(const IRArg& a)
        {
            int cur = current(); if (cur < 0) return;
            std::vector<IRArg> args; args.push_back(a);
            _ir.emitCommand(cur, currentProperty(), args);
        }

        void registerScalars()
        {
            registerType<char>([this](ValueSerializer<char>& s) { emitProp(toIRArg(s._v0)); });
            registerType<signed char>([this](ValueSerializer<signed char>& s) { emitProp(toIRArg(s._v0)); });
            registerType<unsigned char>([this](ValueSerializer<unsigned char>& s) { emitProp(toIRArg(s._v0)); });
            registerType<short>([this](ValueSerializer<short>& s) { emitProp(toIRArg(s._v0)); });
            registerType<unsigned short>([this](ValueSerializer<unsigned short>& s) { emitProp(toIRArg(s._v0)); });
            registerType<int>([this](ValueSerializer<int>& s) { emitProp(toIRArg(s._v0)); });
            registerType<unsigned int>([this](ValueSerializer<unsigned int>& s) { emitProp(toIRArg(s._v0)); });
            registerType<long>([this](ValueSerializer<long>& s) { emitProp(toIRArg(s._v0)); });
            registerType<unsigned long>([this](ValueSerializer<unsigned long>& s) { emitProp(toIRArg(s._v0)); });
            registerType<long long>([this](ValueSerializer<long long>& s) { emitProp(toIRArg(s._v0)); });
            registerType<unsigned long long>([this](ValueSerializer<unsigned long long>& s) { emitProp(toIRArg(s._v0)); });
            registerType<float>([this](ValueSerializer<float>& s) { emitProp(toIRArg(s._v0)); });
            registerType<double>([this](ValueSerializer<double>& s) { emitProp(toIRArg(s._v0)); });

#define REG_VEC(T) registerType<osg::T>([this](ValueSerializer<osg::T>& s) { emitProp(toIRArg(s._v0)); });
            REG_VEC(Vec2b) REG_VEC(Vec2ub) REG_VEC(Vec2s) REG_VEC(Vec2us) REG_VEC(Vec2i) REG_VEC(Vec2ui)
            REG_VEC(Vec2f) REG_VEC(Vec2d)
            REG_VEC(Vec3b) REG_VEC(Vec3ub) REG_VEC(Vec3s) REG_VEC(Vec3us) REG_VEC(Vec3i) REG_VEC(Vec3ui)
            REG_VEC(Vec3f) REG_VEC(Vec3d)
            REG_VEC(Vec4b) REG_VEC(Vec4ub) REG_VEC(Vec4s) REG_VEC(Vec4us) REG_VEC(Vec4i) REG_VEC(Vec4ui)
            REG_VEC(Vec4f) REG_VEC(Vec4d)
#undef REG_VEC
            registerType<osg::Quat>([this](ValueSerializer<osg::Quat>& s) { emitProp(toIRArg(s._v0)); });
            registerType<osg::Plane>([this](ValueSerializer<osg::Plane>& s) { emitProp(toIRArg(s._v0)); });
            registerType<osg::Matrixd>([this](ValueSerializer<osg::Matrixd>& s) { emitProp(toIRArg(s._v0)); });
            registerType<osg::Matrixf>([this](ValueSerializer<osg::Matrixf>& s) { emitProp(toIRArg(s._v0)); });
            registerType<osg::BoundingBoxf>([this](ValueSerializer<osg::BoundingBoxf>& s) { emitProp(toIRArg(s._v0)); });
            registerType<osg::BoundingBoxd>([this](ValueSerializer<osg::BoundingBoxd>& s) { emitProp(toIRArg(s._v0)); });
            registerType<osg::BoundingSpheref>([this](ValueSerializer<osg::BoundingSpheref>& s) { emitProp(toIRArg(s._v0)); });
            registerType<osg::BoundingSphered>([this](ValueSerializer<osg::BoundingSphered>& s) { emitProp(toIRArg(s._v0)); });
        }

        SceneIR& _ir;
        std::vector<int> _stack;
    };
}

bool readSceneIR(const std::string& filename, SceneIR& ir, bool forceAscii)
{
    if (!loadRewrappers()) return false;

    IRRecorderVisitor recorder(ir);
    InputUserData ud;
    ud.setVisitor(&recorder);

    ObjectTypeAndID oid = readObjectFile(filename, ud, forceAscii);
    return oid.valid();
}

}
