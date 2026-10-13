#ifndef GENERIC_INPUTSTREAM_H
#define GENERIC_INPUTSTREAM_H

#include "GenericMath.h"
#include "SceneIR.h"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <memory>

namespace osgVerse
{

// ---------------------------------------------------------------------------
// File format constants (aligned with include/osgDB/DataTypes and the osg serialization format)
// ---------------------------------------------------------------------------
const unsigned int CHAR_SIZE   = 1;
const unsigned int SHORT_SIZE  = 2;
const unsigned int INT_SIZE    = 4;
const unsigned int LONG_SIZE   = 4;   // 'long' is fixed at 4 bytes in the file format
const unsigned int GLENUM_SIZE = 4;
const unsigned int INT64_SIZE  = 8;
const unsigned int FLOAT_SIZE  = 4;
const unsigned int DOUBLE_SIZE = 8;

const int INDENT_VALUE = 2;

const int ID_BYTE_ARRAY   = 0;
const int ID_UBYTE_ARRAY  = 1;
const int ID_SHORT_ARRAY  = 2;
const int ID_USHORT_ARRAY = 3;
const int ID_INT_ARRAY    = 4;
const int ID_UINT_ARRAY   = 5;
const int ID_FLOAT_ARRAY  = 6;
const int ID_DOUBLE_ARRAY = 7;
const int ID_VEC2B_ARRAY  = 8;
const int ID_VEC3B_ARRAY  = 9;
const int ID_VEC4B_ARRAY  = 10;
const int ID_VEC4UB_ARRAY = 11;
const int ID_VEC2S_ARRAY  = 12;
const int ID_VEC3S_ARRAY  = 13;
const int ID_VEC4S_ARRAY  = 14;
const int ID_VEC2_ARRAY   = 15;
const int ID_VEC3_ARRAY   = 16;
const int ID_VEC4_ARRAY   = 17;
const int ID_VEC2D_ARRAY  = 18;
const int ID_VEC3D_ARRAY  = 19;
const int ID_VEC4D_ARRAY  = 20;
const int ID_VEC2UB_ARRAY = 21;
const int ID_VEC3UB_ARRAY = 22;
const int ID_VEC2US_ARRAY = 23;
const int ID_VEC3US_ARRAY = 24;
const int ID_VEC4US_ARRAY = 25;
const int ID_VEC2I_ARRAY  = 26;
const int ID_VEC3I_ARRAY  = 27;
const int ID_VEC4I_ARRAY  = 28;
const int ID_VEC2UI_ARRAY = 29;
const int ID_VEC3UI_ARRAY = 30;
const int ID_VEC4UI_ARRAY = 31;
const int ID_UINT64_ARRAY = 32;
const int ID_INT64_ARRAY  = 33;

const int ID_DRAWARRAYS          = 50;
const int ID_DRAWARRAY_LENGTH    = 51;
const int ID_DRAWELEMENTS_UBYTE  = 52;
const int ID_DRAWELEMENTS_USHORT = 53;
const int ID_DRAWELEMENTS_UINT   = 54;

// Image storage decisions (IMAGE_USE_FILE / IMAGE_INLINE_DATA / IMAGE_INLINE_FILE)
// are provided by SceneIR.h, which this header includes.

const unsigned int OSG_HEADER_LOW  = 0x6C910EA1;
const unsigned int OSG_HEADER_HIGH = 0x1AFB4545;

// Note: GL constants (e.g. GL_POINTS) are provided centrally by GenericMath.h
//     via macros, and are not redefined here (to avoid macro expansion
//     breaking the const declarations).

// ---------------------------------------------------------------------------
// Basic helper types
// ---------------------------------------------------------------------------
class InputStream;
class InputUserData;
class SerializerVisitor;

class InputException : public osg::Referenced
{
public:
    InputException() : osg::Referenced(), _message("") {}
    InputException(const std::string& message) : osg::Referenced(), _message(message) {}
    const std::string& getMessage() const { return _message; }
protected:
    std::string _message;
};

class ObjectGLenum : public osg::Referenced
{
public:
    ObjectGLenum(unsigned int value = 0, const char* type = NULL)
        : osg::Referenced(), _value(value), _type(type ? type : "") {}
    void set(unsigned int value) { _value = value; }
    unsigned int get() const { return _value; }
    void setType(const std::string& t) { _type = t; }
    const std::string& getType() const { return _type; }
    bool operator==(const ObjectGLenum& rhs) const { return _value == rhs._value; }
protected:
    unsigned int _value;
    std::string _type;
};

class ObjectProperty : public osg::Referenced
{
public:
    ObjectProperty(const char* name = NULL, int value = 0, bool mapProperty = false)
        : osg::Referenced(), _name(name ? name : ""), _value(value), _mapProperty(mapProperty) {}
    ObjectProperty(const std::string& name, int value = 0, bool mapProperty = false)
        : osg::Referenced(), _name(name), _value(value), _mapProperty(mapProperty) {}
    void setName(const std::string& name) { _name = name; }
    const std::string& getName() const { return _name; }
    void setValue(int value) { _value = value; }
    int get() const { return _value; }
    std::string _name;
    int _value;
    bool _mapProperty;
};

class ObjectMark : public osg::Referenced
{
public:
    ObjectMark(const char* name = NULL, int mark = 0)
        : osg::Referenced(), _name(name ? name : ""), _mark(mark) {}
    ObjectMark(const std::string& name, int mark = 0)
        : osg::Referenced(), _name(name), _mark(mark) {}
    void setName(const std::string& name) { _name = name; }
    const std::string& getName() const { return _name; }
    void setMark(int mark) { _mark = mark; }
    int getMark() const { return _mark; }
    std::string _name;
    int _mark;
};

class IntLookup
{
public:
    typedef std::map<std::string, int> NameMap;
    typedef std::map<int, std::string> EnumMap;

    void add(const std::string& name, int value)
    { _nameMap[name] = value; _enumMap[value] = name; }
    void add2(const std::string& name, int value)
    { _nameMap[name] = value; }

    int value(const std::string& name) const
    {
        NameMap::const_iterator itr = _nameMap.find(name);
        return itr != _nameMap.end() ? itr->second : 0;
    }
    const std::string& value(int value) const
    {
        EnumMap::const_iterator itr = _enumMap.find(value);
        return itr != _enumMap.end() ? itr->second : _empty;
    }
    bool hasName(const std::string& name) const
    { return _nameMap.find(name) != _nameMap.end(); }

    const NameMap& getNameMap() const { return _nameMap; }
    const EnumMap& getEnumMap() const { return _enumMap; }

protected:
    NameMap _nameMap;
    EnumMap _enumMap;
    static const std::string _empty;
};

// User lookup table proxy: supports two usages
//   1) Global registry: UserLookupTableProxy::get("NAME") accesses the
//      createDefaultTables initialization plus user tables
//   2) Local instance:  static UserLookupTableProxy s_table(&add_user_value_func_XXX);
//      (for USER_READ_FUNC compatibility)
class UserLookupTableProxy
{
public:
    typedef std::map<std::string, IntLookup> LookupTable;
    typedef void (*LookupInitFunc)(IntLookup*);

    UserLookupTableProxy(LookupInitFunc func = NULL) { if (func) func(&_lookup); }
    virtual ~UserLookupTableProxy() {}

    IntLookup& lookup() { return _lookup; }
    const IntLookup& lookup() const { return _lookup; }

    static LookupTable& instance();
    static IntLookup& get(const char* name);

protected:
    IntLookup _lookup;
};

// BEGIN_USER_TABLE / ADD_USER_VALUE / END_USER_TABLE
// Expands to: static void add_user_value_func_##NAME(IntLookup* lookup) { ... }
// The generic version has no real osg class; enum values directly reference
// the global scope (the CLASS parameter is kept only as a placeholder).
#define BEGIN_USER_TABLE(NAME, CLASS) \
    static void add_user_value_func_##NAME(osgVerse::IntLookup* lookup) \
    {
#define ADD_USER_VALUE(VALUE) \
    lookup->add(#VALUE, VALUE);
#define END_USER_TABLE() }

// USER_READ_FUNC: on first invocation, registers add_user_value_func_##NAME into the global table, then looks up the value by name.
#define USER_READ_FUNC(NAME, FUNCNAME) \
    static int FUNCNAME(osgVerse::InputStream& is) { \
        static bool _registered_##FUNCNAME = \
            (add_user_value_func_##NAME(&UserLookupTableProxy::get(#NAME)), true); \
        (void)_registered_##FUNCNAME; \
        int value = 0; if (is.isBinary()) is >> value; \
        else { std::string str; is >> str; \
        value = UserLookupTableProxy::get(#NAME).value(str.c_str()); } \
        return value; }

#define DEF_GLENUM(NAME) osgVerse::ObjectGLenum NAME
#define GLENUM(value) (value)
#define DEF_MAPPEE(NAME, VARIABLE) \
    osgVerse::ObjectProperty VARIABLE(#NAME, 0, true)

// ---------------------------------------------------------------------------
// InputIterator: polymorphic input iterator
// ---------------------------------------------------------------------------
class InputIterator : public osg::Referenced
{
public:
    InputIterator();
    virtual ~InputIterator();

    void setInputStream(InputStream* is) { _inputStream = is; }
    InputStream* getInputStream() const { return _inputStream; }

    virtual bool isBinary() const = 0;
    virtual std::streampos tellg() const = 0;

    void setByteSwap(int byteSwap) { _byteSwap = byteSwap; }
    int getByteSwap() const { return _byteSwap; }

    void setSupportBinaryBrackets(bool b) { _supportBinaryBrackets = b; }
    bool getSupportBinaryBrackets() const { return _supportBinaryBrackets; }

    // Binary array data read (in ASCII the caller reads element-by-element)
    void readComponentArray(char* s, unsigned int numElements,
                            unsigned int numComponentsPerElement,
                            unsigned int componentSize);

    virtual void checkStream() const;
    virtual void throwException(const std::string& msg);
    bool isFailed() const { return _failed; }

    virtual void readBool(bool& b) = 0;
    virtual void readChar(char& c) = 0;
    virtual void readSChar(signed char& c) = 0;
    virtual void readUChar(unsigned char& c) = 0;
    virtual void readShort(short& s) = 0;
    virtual void readUShort(unsigned short& s) = 0;
    virtual void readInt(int& i) = 0;
    virtual void readUInt(unsigned int& i) = 0;
    virtual void readLong(long& l) = 0;
    virtual void readULong(unsigned long& l) = 0;
    virtual void readFloat(float& f) = 0;
    virtual void readDouble(double& d) = 0;
    virtual void readString(std::string& str) = 0;
    virtual void readStream(std::istream& (*fn)(std::istream&)) = 0;
    virtual void readBase(std::ios_base& (*fn)(std::ios_base&)) = 0;
    virtual void readGLenum(ObjectGLenum& value) = 0;
    virtual void readProperty(ObjectProperty& prop) = 0;
    virtual void readMark(ObjectMark& mark) = 0;
    virtual void readCharArray(char* s, unsigned int size) = 0;
    virtual void readWrappedString(std::string& str) = 0;
    virtual bool matchString(const std::string& str) = 0;
    virtual void advanceToCurrentEndBracket() = 0;

protected:
    std::istream* _in;
    InputStream* _inputStream;
    int _byteSwap;
    bool _supportBinaryBrackets;
    mutable bool _failed;
};

class AsciiInputIterator : public InputIterator
{
public:
    AsciiInputIterator(std::istream* input) { _in = input; }
    virtual ~AsciiInputIterator() {}

    virtual bool isBinary() const { return false; }
    virtual std::streampos tellg() const { return _in ? _in->tellg() : std::streampos(-1); }

    virtual void readBool(bool& b);
    virtual void readChar(char& c);
    virtual void readSChar(signed char& c);
    virtual void readUChar(unsigned char& c);
    virtual void readShort(short& s);
    virtual void readUShort(unsigned short& s);
    virtual void readInt(int& i);
    virtual void readUInt(unsigned int& i);
    virtual void readLong(long& l);
    virtual void readULong(unsigned long& l);
    virtual void readFloat(float& f);
    virtual void readDouble(double& d);
    virtual void readString(std::string& str);
    virtual void readStream(std::istream& (*fn)(std::istream&)) {}
    virtual void readBase(std::ios_base& (*fn)(std::ios_base&)) {}
    virtual void readGLenum(ObjectGLenum& value);
    virtual void readProperty(ObjectProperty& prop);
    virtual void readMark(ObjectMark& mark);
    virtual void readCharArray(char* s, unsigned int size) {}
    virtual void readWrappedString(std::string& str);
    virtual bool matchString(const std::string& str);
    virtual void advanceToCurrentEndBracket();

protected:
    void readStringImpl(std::string& str);
    bool isSpecialToken(const std::string& str);

    std::string _preReadString;
};

class BinaryInputIterator : public InputIterator
{
public:
    BinaryInputIterator(std::istream* input) { _in = input; }
    virtual ~BinaryInputIterator() {}

    virtual bool isBinary() const { return true; }
    virtual std::streampos tellg() const { return _in ? _in->tellg() : std::streampos(-1); }

    virtual void readBool(bool& b);
    virtual void readChar(char& c);
    virtual void readSChar(signed char& c);
    virtual void readUChar(unsigned char& c);
    virtual void readShort(short& s);
    virtual void readUShort(unsigned short& s);
    virtual void readInt(int& i);
    virtual void readUInt(unsigned int& i);
    virtual void readLong(long& l);
    virtual void readULong(unsigned long& l);
    virtual void readFloat(float& f);
    virtual void readDouble(double& d);
    virtual void readString(std::string& str);
    virtual void readStream(std::istream& (*fn)(std::istream&)) {}
    virtual void readBase(std::ios_base& (*fn)(std::ios_base&)) {}
    virtual void readGLenum(ObjectGLenum& value);
    virtual void readProperty(ObjectProperty& prop);
    virtual void readMark(ObjectMark& mark);
    virtual void readCharArray(char* s, unsigned int size);
    virtual void readWrappedString(std::string& str);
    virtual bool matchString(const std::string& str) { return false; }
    virtual void advanceToCurrentEndBracket();

protected:
    std::vector<std::streampos> _beginPositions;
    std::vector<std::streampos> _blockSizes;
};

// ---------------------------------------------------------------------------
// ObjectTypeAndID: generic object reference
// ---------------------------------------------------------------------------
class ObjectTypeAndID
{
public:
    ObjectTypeAndID(const std::string& type = "", const std::string& id = "")
        : _type(type), _id(id) {}
    bool valid() const { return !_type.empty(); }

    // Handle-like conveniences so ported wrapper code that used a ref_ptr keeps
    // compiling: the value acts as a smart pointer to itself.
    operator bool() const { return valid(); }
    ObjectTypeAndID* get() { return this; }
    const ObjectTypeAndID* get() const { return this; }
    ObjectTypeAndID* operator->() { return this; }
    const ObjectTypeAndID* operator->() const { return this; }

    std::string _type;
    std::string _id;
};

// ---------------------------------------------------------------------------
// InputStream
// ---------------------------------------------------------------------------
class InputStream
{
public:
    enum ReadType { READ_UNKNOWN, READ_SCENE, READ_IMAGE };

    InputStream();
    virtual ~InputStream();

    typedef std::map<std::string, int> DomainVersionMap;
    typedef std::map<unsigned int, ObjectTypeAndID> IdentifierMap;
    typedef std::map<unsigned int, ObjectTypeAndID> ArrayMap;
    typedef std::map<unsigned int, ObjectTypeAndID> ImageMap;
    typedef std::map<unsigned int, ObjectTypeAndID> ObjectMap;

    void setInputIterator(InputIterator* in) { _in = in; }
    InputIterator* getInputIterator() const { return _in; }

    int getException() const { return _exception; }
    void setException(int e) { _exception = e; }
    void throwException(const std::string& msg);

    void setFileVersion(const std::string& domain, int version);
    int getFileVersion(const std::string& domain = "") const;

    ReadType start(InputIterator* iterator);
    void decompress();

    bool isBinary() const { return _in ? _in->isBinary() : false; }
    std::streampos tellg() const { return _in ? _in->tellg() : std::streampos(-1); }
    void checkStream() const { if (_in) _in->checkStream(); }
    void setByteSwap(int byteSwap) { if (_in) _in->setByteSwap(byteSwap); }
    bool matchString(const std::string& str) { return _in ? _in->matchString(str) : false; }
    void readWrappedString(std::string& str) { if (_in) _in->readWrappedString(str); }
    void readComponentArray(char* memory, unsigned int numElements,
                            unsigned int numComponentsPerElement,
                            unsigned int componentSize)
    { if (_in) _in->readComponentArray(memory, numElements, numComponentsPerElement, componentSize); }
    void readCharArray(char* s, unsigned int size) { if (_in) _in->readCharArray(s, size); }

    ObjectProperty& PROPERTY(const std::string& name, int value = 0);

    unsigned int readSize();

    ObjectTypeAndID readObject(InputUserData& ud);
    ObjectTypeAndID readObjectFields(const std::string& className, const std::string& id, InputUserData& ud);
    ObjectTypeAndID readArray(InputUserData& ud);
    ObjectTypeAndID readPrimitiveSet(InputUserData& ud);
    ObjectTypeAndID readImage(InputUserData& ud);
    ObjectTypeAndID readObjectFromStream(InputUserData& ud, const std::string& type);

    IdentifierMap& getIdentifierMap() { return _identifierMap; }
    ArrayMap& getArrayMap() { return _arrayMap; }
    ImageMap& getImageMap() { return _imageMap; }
    ObjectMap& getObjectMap() { return _objectMap; }

    // ---- operator>> ----
    InputStream& operator>>(bool& b);
    InputStream& operator>>(char& c);
    InputStream& operator>>(signed char& c);
    InputStream& operator>>(unsigned char& c);
    InputStream& operator>>(short& s);
    InputStream& operator>>(unsigned short& s);
    InputStream& operator>>(int& i);
    InputStream& operator>>(unsigned int& i);
    InputStream& operator>>(long& l);
    InputStream& operator>>(unsigned long& l);
    InputStream& operator>>(long long& l);
    InputStream& operator>>(unsigned long long& l);
    InputStream& operator>>(float& f);
    InputStream& operator>>(double& d);
    InputStream& operator>>(std::string& str);
    InputStream& operator>>(InputStream& (*fn)(InputStream&));
    InputStream& operator>>(std::ios_base& (*fn)(std::ios_base&));
    InputStream& operator>>(ObjectGLenum& v);
    InputStream& operator>>(ObjectProperty& p);
    InputStream& operator>>(ObjectMark& m);

    template<unsigned int N, typename T>
    InputStream& operator>>(osg::Vec<N,T>& v)
    {
        if (isBinary())
        {
            _in->readComponentArray((char*)&v._v[0], N, 1, sizeof(T));
            checkStream();
        }
        else
        {
            for (unsigned int i = 0; i < N; ++i) *this >> v._v[i];
        }
        return *this;
    }

    InputStream& operator>>(osg::Quat& q);
    InputStream& operator>>(osg::Plane& p);
    InputStream& operator>>(osg::Matrixd& m);
    InputStream& operator>>(osg::Matrixf& m);
    InputStream& operator>>(osg::BoundingBoxf& bb);
    InputStream& operator>>(osg::BoundingBoxd& bb);
    InputStream& operator>>(osg::BoundingSpheref& bs);
    InputStream& operator>>(osg::BoundingSphered& bs);

    ObjectProperty _property;
    ObjectMark     BEGIN_BRACKET;
    ObjectMark     END_BRACKET;

protected:
    void readArrayImplementation(unsigned int arrayType, unsigned int numComponents,
                                 unsigned int componentSizeInBytes,
                                 std::vector<double>* out = NULL);

    DomainVersionMap _domainVersionMap;
    IdentifierMap    _identifierMap;
    ArrayMap         _arrayMap;
    ImageMap         _imageMap;
    ObjectMap        _objectMap;
    InputIterator*   _in;
    int              _exception;
    std::vector<double>* _arrayCapture;   // optional capture target for readArray (legacy path)
};

// ---------------------------------------------------------------------------
// IR argument conversion: packages the arguments of a recorded command
// (ud.add(...)) into the neutral IR value type. Raw pointers whose payload
// length is carried by a separate argument are not captured.
// ---------------------------------------------------------------------------
inline IRArg toIRArg(bool v) { return IRArg::makeBool(v); }
inline IRArg toIRArg(char v) { return IRArg::makeInt((long long)v); }
inline IRArg toIRArg(signed char v) { return IRArg::makeInt((long long)v); }
inline IRArg toIRArg(unsigned char v) { return IRArg::makeUInt((unsigned long long)v); }
inline IRArg toIRArg(short v) { return IRArg::makeInt((long long)v); }
inline IRArg toIRArg(unsigned short v) { return IRArg::makeUInt((unsigned long long)v); }
inline IRArg toIRArg(int v) { return IRArg::makeInt((long long)v); }
inline IRArg toIRArg(unsigned int v) { return IRArg::makeUInt((unsigned long long)v); }
inline IRArg toIRArg(long v) { return IRArg::makeInt((long long)v); }
inline IRArg toIRArg(unsigned long v) { return IRArg::makeUInt((unsigned long long)v); }
inline IRArg toIRArg(long long v) { return IRArg::makeInt(v); }
inline IRArg toIRArg(unsigned long long v) { return IRArg::makeUInt(v); }
inline IRArg toIRArg(float v) { return IRArg::makeDouble((double)v); }
inline IRArg toIRArg(double v) { return IRArg::makeDouble(v); }
inline IRArg toIRArg(const char* v) { return IRArg::makeString(v ? std::string(v) : std::string()); }
inline IRArg toIRArg(const std::string& v) { return IRArg::makeString(v); }
inline IRArg toIRArg(const ObjectTypeAndID& o) { return IRArg::makeObject(o._type, o._id); }
inline IRArg toIRArg(const ObjectTypeAndID* o)
{ return o ? IRArg::makeObject(o->_type, o->_id) : IRArg(); }
inline IRArg toIRArg(unsigned char*) { return IRArg(); }
inline IRArg toIRArg(const unsigned char*) { return IRArg(); }

template<unsigned int N, typename T>
inline IRArg toIRArg(const osg::Vec<N, T>& v)
{
    IRArg a; a.kind = (N == 2) ? IRArg::VEC2 : (N == 3 ? IRArg::VEC3 : IRArg::VEC4);
    a.n = (int)N;
    for (unsigned int i = 0; i < N && i < 16u; ++i) a.v[i] = (double)v._v[i];
    return a;
}
inline IRArg toIRArg(const osg::Quat& q)
{ IRArg a; a.kind = IRArg::QUAT; a.n = 4; for (int i = 0; i < 4; ++i) a.v[i] = q._v[i]; return a; }
inline IRArg toIRArg(const osg::Plane& p)
{ IRArg a; a.kind = IRArg::PLANE; a.n = 4; for (int i = 0; i < 4; ++i) a.v[i] = p._v[i]; return a; }
inline IRArg toIRArg(const osg::Matrixd& m)
{ IRArg a; a.kind = IRArg::MATRIX; a.n = 16; for (int i = 0; i < 16; ++i) a.v[i] = m._m[i]; return a; }
inline IRArg toIRArg(const osg::Matrixf& m)
{ IRArg a; a.kind = IRArg::MATRIX; a.n = 16; for (int i = 0; i < 16; ++i) a.v[i] = (double)m._m[i]; return a; }
inline IRArg toIRArg(const osg::BoundingBoxf& b)
{ IRArg a; a.kind = IRArg::BBOX; a.n = 6;
  for (int i = 0; i < 3; ++i) { a.v[i] = (double)b._min._v[i]; a.v[3 + i] = (double)b._max._v[i]; } return a; }
inline IRArg toIRArg(const osg::BoundingBoxd& b)
{ IRArg a; a.kind = IRArg::BBOX; a.n = 6;
  for (int i = 0; i < 3; ++i) { a.v[i] = b._min._v[i]; a.v[3 + i] = b._max._v[i]; } return a; }
inline IRArg toIRArg(const osg::BoundingSpheref& s)
{ IRArg a; a.kind = IRArg::BSPHERE; a.n = 4;
  for (int i = 0; i < 3; ++i) a.v[i] = (double)s._center._v[i]; a.v[3] = (double)s._radius; return a; }
inline IRArg toIRArg(const osg::BoundingSphered& s)
{ IRArg a; a.kind = IRArg::BSPHERE; a.n = 4;
  for (int i = 0; i < 3; ++i) a.v[i] = s._center._v[i]; a.v[3] = s._radius; return a; }

// ---------------------------------------------------------------------------
// InputUserData: the target-agnostic sink used while reading.
//
//  * `add(...)` receives the explicit commands emitted by user-serializer read
//    functions (addChild / setMode / ...). They are forwarded to the optional
//    SerializerVisitor and/or recorded into the optional SceneIR.
//  * addTreeNode()/pushTreeNode()/popTreeNode() delimit object scopes and drive
//    the visitor's beginObject()/endObject() callbacks and the IR object list.
//  * Serializer values (basic types, objects, arrays, ...) are delivered to the
//    visitor by InputStream::readSerializerList() through the SerializerVisitor
//    apply() overloads.
// ---------------------------------------------------------------------------
class InputUserData
{
public:
    typedef std::map<unsigned int, ObjectTypeAndID> ObjectMap;

    // Scene-tree node recorded while reading objects (for diagnostics printing):
    // one node per readObject() call; misaligned reads show up immediately as
    // garbled class names with their exact file offsets.
    struct TreeNode
    {
        TreeNode() : parentId(-1), startPos(0) {}
        std::string className;
        std::string id;
        std::streampos startPos;
        int parentId;
        std::vector<int> children;
    };

    InputUserData();
    virtual ~InputUserData();

    // Engine extension point: an optional visitor that rebuilds the scene while
    // the file is read. Not owned by InputUserData.
    void setVisitor(SerializerVisitor* v) { _visitor = v; }
    SerializerVisitor* getVisitor() const { return _visitor; }

    // Optional neutral-IR recording (SceneIR is not owned by InputUserData).
    void setIR(SceneIR* ir) { _ir = ir; }
    SceneIR* getIR() const { return _ir; }
    int currentIR() const { return _irStack.empty() ? -1 : _irStack.back(); }

    template<typename... Args> void add(const char* name, Args&&... args)
    { emitCommand(name, std::vector<IRArg>{ toIRArg(args)... }); }
    template<typename... Args> void add(const char* name, unsigned int index, Args&&... args)
    { emitCommand(name, std::vector<IRArg>{ toIRArg(index), toIRArg(args)... }); }
    template<typename... Args> void add(const ObjectTypeAndID& parent, const char* name, Args&&... args)
    { emitCommand(parent, name, std::vector<IRArg>{ toIRArg(args)... }); }
    template<typename... Args> void add(const ObjectTypeAndID& parent, const char* name, unsigned int index, Args&&... args)
    { emitCommand(parent, name, std::vector<IRArg>{ toIRArg(index), toIRArg(args)... }); }

    // The generic version's get exists only for compile-time compatibility
    // (no real property storage); it always returns false.
    // The variadic template covers the various argument-passing styles used by
    // osg serialization code for get(...) (including passing pointers &value).
    template<typename... Args> bool get(const char* name, Args&&...) const { return false; }
    template<typename... Args> bool get(const ObjectTypeAndID& parent, const char* name, Args&&...) const { return false; }

    ObjectTypeAndID readObjectFromStream(InputStream& is, const std::string& type);

    // Object-scope and data-bearing notifications, called by InputStream while
    // reading (forwarded to the visitor). Public because InputStream owns them.
    void notifyBeginObject(const std::string& className, const std::string& id);
    void notifyEndObject();
    void notifyArrayData(int components, const std::vector<double>& values);
    void notifyPrimitiveData(int mode, unsigned int first, unsigned int count,
                             const std::vector<unsigned int>& indices,
                             const std::vector<unsigned int>& lengths);
    void notifyImageData(const std::string& fileName, int origin,
                         int width, int height, int depth,
                         int internalFormat, int pixelFormat, int dataType,
                         int packing, int allocationMode, int decision,
                         const std::vector<unsigned char>& pixels);

    // Scene-tree recording (called from InputStream::readObject)
    int addTreeNode(const std::string& className, const std::string& id, std::streampos pos)
    {
        TreeNode node; node.className = className; node.id = id; node.startPos = pos;
        node.parentId = _treeStack.empty() ? -1 : _treeStack.back();
        _treeNodes.push_back(node);
        int idx = (int)(_treeNodes.size() - 1);
        if (node.parentId >= 0) _treeNodes[node.parentId].children.push_back(idx);
        if (_ir)
        {
            int parentIr = _irStack.empty() ? -1 : _irStack.back();
            _treeToIr[idx] = _ir->createObject(className, id, parentIr, (long long)pos);
        }
        notifyBeginObject(className, id);
        return idx;
    }
    void pushTreeNode(int idx)
    {
        _treeStack.push_back(idx);
        if (_ir)
        {
            std::map<int, int>::iterator itr = _treeToIr.find(idx);
            _irStack.push_back(itr != _treeToIr.end() ? itr->second : -1);
        }
    }
    void popTreeNode()
    {
        if (!_treeStack.empty()) _treeStack.pop_back();
        if (_ir && !_irStack.empty()) _irStack.pop_back();
        notifyEndObject();
    }

    void clear();

    ObjectMap& getObjectMap() { return _objectMap; }
    const ObjectMap& getObjectMap() const { return _objectMap; }
    const std::vector<TreeNode>& getTreeNodes() const { return _treeNodes; }

protected:
    // Defined in GenericInputStream.cpp (need the complete SerializerVisitor).
    void emitCommand(const char* name, const std::vector<IRArg>& args);
    void emitCommand(const ObjectTypeAndID& parent, const char* name, const std::vector<IRArg>& args);

    ObjectMap _objectMap;
    std::vector<TreeNode> _treeNodes;
    std::vector<int> _treeStack;
    SerializerVisitor* _visitor;
    SceneIR* _ir;
    std::vector<int> _irStack;
    std::map<int, int> _treeToIr;
};

} // namespace osgVerse

#endif
