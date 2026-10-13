#ifndef MANA_SCRIPT_GENERICRESERIALIZER_HPP
#define MANA_SCRIPT_GENERICRESERIALIZER_HPP

#include <vector>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <functional>
#include <typeindex>
#include <typeinfo>
#include <memory>
#include <climits>
#include <cfloat>
#include "GenericMath.h"
#include "GenericInputStream.h"
#include "SceneIR.h"

extern "C"
{
    typedef void (*WrapperLoadingFunction)(void);
}

namespace osgVerse
{
    struct BaseSerializer;
    struct WrappableSerializer;
    struct ObjectSerializer;
    struct ImageSerializer;
    struct VectorSerializer;
    struct UserSerializer;
    struct BoolSerializer;
    struct GLenumSerializer;
    struct StringSerializer;
    struct EnumSerializer;
    template<typename T> struct ValueSerializer;
    template<typename T> struct ListSerializer;
    struct BitFlagSerializer;
    class RewrapperManager;

    class SerializerVisitor
    {
    public:
        template<typename T> void apply(ValueSerializer<T>& obj);
        void apply(BaseSerializer& obj) { applyValue(obj); }
        virtual void apply(ObjectSerializer& obj) {}
        virtual void apply(ImageSerializer& obj) {}
        virtual void apply(VectorSerializer& obj) {}
        virtual void apply(UserSerializer& obj) {}
        virtual void apply(BoolSerializer& obj) {}
        virtual void apply(StringSerializer& obj) {}
        virtual void apply(EnumSerializer& obj) {}
        virtual void apply(GLenumSerializer& obj) {}
        virtual void apply(BitFlagSerializer& obj) {}
        virtual void apply(ListSerializer<ObjectTypeAndID>& obj) {}

        template<typename T>
        void registerType(std::function<void(ValueSerializer<T>&)> func)
        {
            _registry[std::type_index(typeid(ValueSerializer<T>))] = [func](BaseSerializer& baseObj)
            { ValueSerializer<T>& obj = static_cast<ValueSerializer<T>&>(baseObj); func(obj); };
        }

        SerializerVisitor() : _manager(NULL), _inputVersion(INT_MAX), _is(NULL), _ud(NULL) {}
        void setManager(RewrapperManager* m, int v) { _manager = m; _inputVersion = v; }
        void setInput(InputStream* is, InputUserData* ud) { _is = is; _ud = ud; }
        InputStream* getInputStream() { return _is; }
        InputUserData* getUserData() { return _ud; }

        // Engine extension points. A visitor subclass rebuilds the scene in a
        // concrete engine: beginObject() creates the engine object, apply()/the
        // serializer overloads set properties from the values already read by
        // the InputStream, command() receives explicit user-serializer commands
        // (addChild, setMode, ...), and endObject() closes the object scope.
        virtual void beginObject(const std::string& className, const std::string& id) {}
        virtual void endObject() {}
        virtual void command(const std::string& name, const std::vector<IRArg>& args) {}

        // Data-bearing objects that the legacy/compact readers (readArray /
        // readPrimitiveSet / readImage) do not expose through the serializer
        // channel. Each is preceded by beginObject(className, id) and followed
        // by endObject(), so a visitor can create/annotate the object.
        virtual void arrayData(int components, const std::vector<double>& values) {}
        virtual void primitiveData(int mode, unsigned int first, unsigned int count,
                                   const std::vector<unsigned int>& indices,
                                   const std::vector<unsigned int>& lengths) {}
        virtual void imageData(const std::string& fileName, int origin,
                               int width, int height, int depth,
                               int internalFormat, int pixelFormat, int dataType,
                               int packing, int allocationMode, int decision,
                               const std::vector<unsigned char>& pixels) {}

        void push(const std::string& n) { _propertyList.push(n); }
        void pop() { _propertyList.pop(); }
        const std::stack<std::string>& getPropertyList() const { return _propertyList; }
        const std::string& currentProperty() const
        { return _propertyList.empty() ? _emptyProperty : _propertyList.top(); }

    protected:
        void traverse(WrappableSerializer& obj);
        void applyValue(BaseSerializer& obj);

        typedef std::map<std::type_index, std::function<void(BaseSerializer&)>> SerializerMap;
        SerializerMap _registry; RewrapperManager* _manager; int _inputVersion;
        InputStream* _is; InputUserData* _ud;
        std::stack<std::string> _propertyList;
        static const std::string _emptyProperty;
    };

    // Type-dispatched read: fills the serializer's value from an InputStream.
    // Returns false if the serializer type is not handled.
    bool readSerializerValue(InputStream& is, InputUserData& ud, BaseSerializer& s);
#define META_VISITOR() virtual void accept(const std::string& n, SerializerVisitor& v) \
                       { v.push(n); v.apply(*this); v.pop(); }

    struct BaseSerializer : public osg::Referenced
    {
        typedef std::function<void(InputStream& is, InputUserData& ud)> ReadFunc;
        BaseSerializer() : _firstVersion(0), _lastVersion(INT_MAX) {}
        int _firstVersion, _lastVersion; META_VISITOR()
    };

    struct WrapperAssociate
    {
        WrapperAssociate(const std::string& name) : _firstVersion(0), _lastVersion(INT_MAX), _name(name) {}
        int _firstVersion, _lastVersion; std::string _name;
    };

    struct WrappableSerializer : public BaseSerializer { std::string _type; };
    struct ObjectSerializer : public WrappableSerializer
    { ObjectTypeAndID _readValue; META_VISITOR() };
    struct ImageSerializer : public WrappableSerializer
    { ObjectTypeAndID _readValue; META_VISITOR() };
    struct VectorSerializer : public BaseSerializer
    { std::string _type; int _numComponents; std::vector<double> _values; META_VISITOR() };
    struct UserSerializer : public BaseSerializer { ReadFunc _reader; META_VISITOR() };
    struct BoolSerializer : public BaseSerializer { bool _v0; META_VISITOR() };
    struct StringSerializer : public BaseSerializer { std::string _v0; META_VISITOR() };
    struct GLenumSerializer : public BaseSerializer { std::string _type; unsigned int _v0; META_VISITOR() };
    template<typename T> struct ValueSerializer : public BaseSerializer { T _v0; bool _hex; META_VISITOR() };
    template<typename T> struct ListSerializer : public BaseSerializer
    { T _v0; std::string _elemType; std::vector<T> _elements; META_VISITOR() };

    struct EnumSerializer : public BaseSerializer
    { std::string _v0; std::set<std::string> _values; META_VISITOR() };

    struct BitFlagSerializer : public BaseSerializer
    { bool _asInt; unsigned int _v0; std::map<std::string, unsigned int> _valueMap; META_VISITOR() };

    template<typename T> void SerializerVisitor::apply(ValueSerializer<T>& obj)
    { applyValue(static_cast<BaseSerializer&>(obj)); }

    class Rewrapper : public osg::Referenced
    {
    public:
        template<typename T> void addValueSerializer(const std::string& prop, T def, bool hex = false)
        { ValueSerializer<T>* s = new ValueSerializer<T>(); s->_v0 = def; s->_hex = hex; add(prop, s); }

        template<typename T> void addRefValueSerializer(const std::string& prop, const T& def, bool hex = false)
        { ValueSerializer<T>* s = new ValueSerializer<T>(); s->_v0 = def; s->_hex = hex; add(prop, s); }

        template<typename T> void addListSerializer(const std::string& prop)
        { ListSerializer<T>* s = new ListSerializer<T>(); add(prop, s); }

        void addObjectListSerializer(const std::string& prop, const std::string& t)
        {
            ListSerializer<ObjectTypeAndID>* s = new ListSerializer<ObjectTypeAndID>();
            s->_v0 = ObjectTypeAndID(t, "");
            s->_elemType = t;  // fixed element type - _v0 may be overwritten while reading
            add(prop, s);
        }

        void addBoolSerializer(const std::string& prop, bool def)
        { BoolSerializer* s = new BoolSerializer; s->_v0 = def; add(prop, s); }

        void addGLenumSerializer(const std::string& prop, const std::string& type, unsigned int def)
        { GLenumSerializer* s = new GLenumSerializer; s->_type = type; s->_v0 = def; add(prop, s); }

        void addStringSerializer(const std::string& prop, const std::string& def)
        { StringSerializer* s = new StringSerializer; s->_v0 = def; add(prop, s); }

        void addObjectSerializer(const std::string& prop, const std::string& type)
        { ObjectSerializer* s = new ObjectSerializer; s->_type = type; add(prop, s); }

        void addImageSerializer(const std::string& prop, const std::string& type)
        { ImageSerializer* s = new ImageSerializer; s->_type = type; add(prop, s); }

        void addVectorSerializer(const std::string& prop, const std::string& type)
        { VectorSerializer* s = new VectorSerializer; s->_type = type; add(prop, s); }

        void addUserSerializer(const std::string& prop, BaseSerializer::ReadFunc f)
        { UserSerializer* s = new UserSerializer; s->_reader = f; add(prop, s); }

        void beginEnumSerializer(const std::string& prop, const std::string& value)
        { _lastEnum = new EnumSerializer; _lastEnum->_v0 = value; add(prop, _lastEnum); }
        void addEnumValue(const std::string & v) { if (_lastEnum) _lastEnum->_values.insert(v); }
        void endEnumSerializer() { _lastEnum = NULL; }

        void beginBitFlagSerializer(const std::string& prop, unsigned int v, bool i)
        { _lastBitFlags = new BitFlagSerializer; _lastBitFlags->_asInt = i; _lastBitFlags->_v0 = v; add(prop, _lastBitFlags); }
        void addBitFlagValue(const std::string& v, unsigned int ui) { if (_lastBitFlags) { _lastBitFlags->_valueMap[v] = ui; } }
        void endBitFlagSerializer() { _lastBitFlags = NULL; }

        void setAssociates(const std::string& a) { splitAssociates(a); }
        void add(const std::string& n, BaseSerializer* s);
        void clear();

        void setUpdatedVersion(int v) { _version = v; }
        int getUpdatedVersion() const { return _version; }

        void markSerializerAsRemoved(const std::string& name);
        void markAssociateAsRemoved(const std::string& name);
        void markAssociateAsAdded(const std::string& name);

        Rewrapper() : _lastBitFlags(NULL), _lastEnum(NULL), _version(0) {}
        void accept(RewrapperManager* manager, SerializerVisitor& v,
                    int inputVersion, bool includingAssociates = true);

        typedef std::pair<std::string, osg::ref_ptr<BaseSerializer>> SerializerPair;
        const std::vector<SerializerPair>& getSerializers() const { return _serializers; }
        const std::vector<WrapperAssociate>& getAssociates() const { return _associates; }

    protected:
        virtual ~Rewrapper() {}
        void splitAssociates(const std::string& associates);

        std::vector<SerializerPair> _serializers;
        std::vector<WrapperAssociate> _associates;
        BitFlagSerializer* _lastBitFlags;
        EnumSerializer* _lastEnum; int _version;
    };

    class RewrapperManager
    {
    public:
        static std::shared_ptr<RewrapperManager> instance();
        virtual ~RewrapperManager();

        void addRewrapper(const std::string& name, Rewrapper* r);
        void removeRewrapper(const std::string& name);
        Rewrapper* getRewrapper(const std::string& name);

        typedef std::map<std::string, osg::ref_ptr<Rewrapper>> RewrapperMap;
        const RewrapperMap& getRewrappers() const { return _rewrappers; }

    protected:
        RewrapperMap _rewrappers;
    };

    struct RegisterWrapperProxy
    {
    public:
        typedef void (*AddPropFunc)(Rewrapper&);
        RegisterWrapperProxy(const std::string& name, const std::string& associates, AddPropFunc func);
        virtual ~RegisterWrapperProxy(); std::string _className;
    };

    struct RegisterWrapperFunctionProxy
    {
        RegisterWrapperFunctionProxy(WrapperLoadingFunction function) { (function)(); }
    };

    struct UpdateWrapperVersionProxy
    {
        UpdateWrapperVersionProxy(Rewrapper* w, int v) : _wrapper(w)
        { _lastVersion = w->getUpdatedVersion(); w->setUpdatedVersion(v); }

        ~UpdateWrapperVersionProxy() { _wrapper->setUpdatedVersion(_lastVersion); }
        Rewrapper* _wrapper; int _lastVersion;
    };
}

#define ADD_USER_SERIALIZER(PROP) wrapper.addUserSerializer(#PROP, &read##PROP)
#define ADD_BOOL_SERIALIZER(PROP, DEF) wrapper.addBoolSerializer(#PROP, DEF)
#define ADD_GLENUM_SERIALIZER(PROP, TYPE, DEF) wrapper.addGLenumSerializer(#PROP, #TYPE, DEF)
#define ADD_STRING_SERIALIZER(PROP, DEF) wrapper.addStringSerializer(#PROP, DEF)
#define ADD_OBJECT_SERIALIZER(PROP, TYPE, DEF) wrapper.addObjectSerializer(#PROP, #TYPE)
#define ADD_IMAGE_SERIALIZER(PROP, TYPE, DEF) wrapper.addImageSerializer(#PROP, #TYPE)

#define ADD_CHAR_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<char>(#PROP, DEF)
#define ADD_UCHAR_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned char>(#PROP, DEF)
#define ADD_SHORT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<short>(#PROP, DEF)
#define ADD_USHORT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned short>(#PROP, DEF)
#define ADD_HEXSHORT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned short>(#PROP, DEF, true)
#define ADD_INT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<int>(#PROP, DEF)
#define ADD_UINT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned int>(#PROP, DEF)
#define ADD_GLINT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<GLint>(#PROP, DEF)
#define ADD_HEXINT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<unsigned int>(#PROP, DEF, true)
#define ADD_FLOAT_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<float>(#PROP, DEF)
#define ADD_DOUBLE_SERIALIZER(PROP, DEF) wrapper.addValueSerializer<double>(#PROP, DEF)
#define ADD_VEC2B_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2b>(#PROP, DEF)
#define ADD_VEC2UB_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2ub>(#PROP, DEF)
#define ADD_VEC2S_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2s>(#PROP, DEF)
#define ADD_VEC2US_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2us>(#PROP, DEF)
#define ADD_VEC2I_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2i>(#PROP, DEF)
#define ADD_VEC2UI_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2ui>(#PROP, DEF)
#define ADD_VEC2F_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2f>(#PROP, DEF)
#define ADD_VEC2D_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec2d>(#PROP, DEF)
#define ADD_VEC2_SERIALIZER(PROP, DEF) ADD_VEC2F_SERIALIZER(PROP, DEF)
#define ADD_VEC3B_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3b>(#PROP, DEF)
#define ADD_VEC3UB_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3ub>(#PROP, DEF)
#define ADD_VEC3S_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3s>(#PROP, DEF)
#define ADD_VEC3US_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3us>(#PROP, DEF)
#define ADD_VEC3I_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3i>(#PROP, DEF)
#define ADD_VEC3UI_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3ui>(#PROP, DEF)
#define ADD_VEC3F_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3f>(#PROP, DEF)
#define ADD_VEC3D_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec3d>(#PROP, DEF)
#define ADD_VEC3_SERIALIZER(PROP, DEF) ADD_VEC3F_SERIALIZER(PROP, DEF)
#define ADD_VEC4B_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4b>(#PROP, DEF)
#define ADD_VEC4UB_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4ub>(#PROP, DEF)
#define ADD_VEC4S_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4s>(#PROP, DEF)
#define ADD_VEC4US_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4us>(#PROP, DEF)
#define ADD_VEC4I_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4i>(#PROP, DEF)
#define ADD_VEC4UI_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4ui>(#PROP, DEF)
#define ADD_VEC4F_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4f>(#PROP, DEF)
#define ADD_VEC4D_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Vec4d>(#PROP, DEF)
#define ADD_VEC4_SERIALIZER(PROP, DEF) ADD_VEC4F_SERIALIZER(PROP, DEF)
#define ADD_QUAT_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Quat>(#PROP, DEF)
#define ADD_PLANE_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Plane>(#PROP, DEF)
#define ADD_MATRIX_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Matrix>(#PROP, DEF)
#define ADD_MATRIXF_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Matrixf>(#PROP, DEF)
#define ADD_MATRIXD_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::Matrixd>(#PROP, DEF)
#define ADD_BOUNDINGBOXF_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingBoxf>(#PROP, DEF)
#define ADD_BOUNDINGBOXD_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingBoxd>(#PROP, DEF)
#define ADD_BOUNDINGSPHEREF_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingSpheref>(#PROP, DEF)
#define ADD_BOUNDINGSPHERED_SERIALIZER(PROP, DEF) wrapper.addRefValueSerializer<osg::BoundingSphered>(#PROP, DEF)

#define ADD_VECTOR_SERIALIZER(PROP, TYPE, ETYPE, NUMONROW) wrapper.addVectorSerializer(#PROP, #TYPE)
#define ADD_ISAVECTOR_SERIALIZER(PROP, ETYPE, NUMONROW) wrapper.addVectorSerializer(#PROP, #ETYPE)
#define ADD_LIST_SERIALIZER(PROP, TYPE) wrapper.addListSerializer<TYPE>(#PROP);
#define ADD_OBJECT_LIST_SERIALIZER(PROP, TYPE) wrapper.addObjectListSerializer(#PROP, #TYPE);

#define BEGIN_ENUM_SERIALIZER(PROP, DEF) wrapper.beginEnumSerializer(#PROP, #DEF);
#define BEGIN_ENUM_SERIALIZER2(PROP, TYPE, DEF) wrapper.beginEnumSerializer(#PROP, #DEF);
#define ADD_ENUM_VALUE(VALUE) wrapper.addEnumValue(#VALUE);
#define END_ENUM_SERIALIZER() wrapper.endEnumSerializer();

#define BEGIN_BITFLAGS_SERIALIZER(PROP, DEF) wrapper.beginBitFlagSerializer(#PROP, DEF, false);
#define BEGIN_UINT_BITFLAGS_SERIALIZER(PROP, DEF) wrapper.beginBitFlagSerializer(#PROP, DEF, false);
#define BEGIN_INT_BITFLAGS_SERIALIZER(PROP, DEF) wrapper.beginBitFlagSerializer(#PROP, DEF, true);
#define ADD_BITFLAG_VALUE(VALUE_NAME, VALUE) wrapper.addBitFlagValue(#VALUE_NAME, VALUE);
#define END_BITFLAGS_SERIALIZER() wrapper.endBitFlagSerializer();

#define UPDATE_TO_VERSION(VER) wrapper.setUpdatedVersion((VER));
#define UPDATE_TO_VERSION_SCOPED(VER) UpdateWrapperVersionProxy uwvp(&wrapper, (VER));
#define ADDED_ASSOCIATE(STR) wrapper.markAssociateAsAdded(STR);
#define REMOVED_ASSOCIATE(STR) wrapper.markAssociateAsRemoved(STR);
#define REMOVE_SERIALIZER(PROP) wrapper.markSerializerAsRemoved(#PROP);

// ---------------------------------------------------------------------------
// Aliases for the extra spellings used by the non-core osg serializers, so the
// upstream wrapper sources port over without per-file edits. The scoped-enum /
// type arguments are only used for scripting on the osg side and are dropped.
// ---------------------------------------------------------------------------
#define BEGIN_ENUM_SERIALIZER3(TYPE, PROP, DEF) wrapper.beginEnumSerializer(#PROP, #DEF)
#define BEGIN_ENUM_SERIALIZER4(TYPE, PROP, DEF) wrapper.beginEnumSerializer(#PROP, #DEF)
#define ADD_ENUM_CLASS_VALUE(TYPE, VALUE) wrapper.addEnumValue(#VALUE)
#define ADD_ENUM_CLASS_VALUE_CAST(TYPE, VALUE) wrapper.addEnumValue(#VALUE)
#define ADD_OBJECT_SERIALIZER_NO_SET(PROP, TYPE, DEF) wrapper.addObjectSerializer(#PROP, #TYPE)
#define ADD_IMAGE_SERIALIZER_NO_SET(PROP, TYPE, DEF) wrapper.addImageSerializer(#PROP, #TYPE)
#define ADD_UINT_SERIALIZER_NO_SET(PROP, DEF) wrapper.addValueSerializer<unsigned int>(#PROP, DEF)
#define ADD_INT_SERIALIZER_NO_SET(PROP, DEF) wrapper.addValueSerializer<int>(#PROP, DEF)
#define ADD_STRING_SERIALIZER_NO_SET(PROP, DEF) wrapper.addStringSerializer(#PROP, DEF)
#define USE_SERIALIZER_WRAPPER(NAME) USE_OBJECT_WRAPPER(NAME)

// Scripting-only hooks (osgDB::MethodObject tables). The generic layer has no
// scripting engine, so they expand to nothing; the matching MethodObject struct
// definitions are dropped while porting.
#define ADD_METHOD(NAME)
#define ADD_METHOD_OBJECT(NAME, CLASS)
#define SET_USAGE(VALUE)

#define REGISTER_OBJECT_WRAPPER(NAME, CREATEINSTANCE, CLASS, ASSOCIATES) \
    extern "C" void rewrapper_serializer_##NAME(void) {} \
    extern void rewrapper_propfunc_##NAME(Rewrapper& wrapper); \
    static osgVerse::RegisterWrapperProxy rewrapper_proxy_##NAME(#CLASS, ASSOCIATES, &rewrapper_propfunc_##NAME); \
    void rewrapper_propfunc_##NAME(Rewrapper& wrapper)
// Variant used by a few osg wrappers that also pass the class name explicitly.
#define REGISTER_OBJECT_WRAPPER2(NAME, CREATEINSTANCE, CLASS, CLASSNAME, ASSOCIATES) \
    REGISTER_OBJECT_WRAPPER(NAME, CREATEINSTANCE, CLASS, ASSOCIATES)
#define USE_OBJECT_WRAPPER(NAME) \
    extern "C" void rewrapper_serializer_##NAME(void); \
    static osgVerse::RegisterWrapperFunctionProxy rewrapperfunc_proxy_##NAME(rewrapper_serializer_##NAME);

// Static-archive builds keep the registrars alive through
// USE_SERIALIZER_WRAPPER_LIBRARY() in GenericStaticWrappers.h. That header is
// self-contained on purpose (it must not pull in the mini-osg math layer), so
// the macro lives there rather than next to USE_OBJECT_WRAPPER.

#endif
