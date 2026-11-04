#ifndef GENERIC_OUTPUTSTREAM_H
#define GENERIC_OUTPUTSTREAM_H

// GenericOutputStream: the write-side counterpart of GenericInputStream.
//
// It mirrors osgDB's OutputStream / OutputIterator pair, but instead of asking
// real osg objects for their values it writes a *SceneIR*: the neutral object
// graph produced by the reader (or built by a host application).  For every
// wrapper property the value is looked up in the IR's recorded commands, so the
// same `writeXxx` functions can turn an IR back into .osgt/.osgb bytes.
//
// Typical use (see GenericWriter.h for the convenience entry points):
//
//   OutputStream os;
//   os.setFileVersion(161);
//   os.start(iterator, OutputStream::WRITE_SCENE);
//   OutputUserData ud(ir);
//   os.writeObject(ud, rootIndex);

#include "GenericInputStream.h"
#include "SceneIR.h"

#include <ostream>
#include <string>
#include <vector>
#include <map>

namespace osgVerse
{

class OutputStream;

// ---------------------------------------------------------------------------
// OutputIterator: polymorphic output iterator (ASCII text / binary bytes)
// ---------------------------------------------------------------------------
class OutputIterator : public osg::Referenced
{
public:
    OutputIterator();
    virtual ~OutputIterator();

    void setOutputStream(OutputStream* os) { _outputStream = os; }
    OutputStream* getOutputStream() const { return _outputStream; }

    void setSupportBinaryBrackets(bool b) { _supportBinaryBrackets = b; }
    bool getSupportBinaryBrackets() const { return _supportBinaryBrackets; }

    virtual bool isBinary() const = 0;

    virtual void writeBool(bool b) = 0;
    virtual void writeChar(char c) = 0;
    virtual void writeSChar(signed char c) = 0;
    virtual void writeUChar(unsigned char c) = 0;
    virtual void writeShort(short s) = 0;
    virtual void writeUShort(unsigned short s) = 0;
    virtual void writeInt(int i) = 0;
    virtual void writeUInt(unsigned int i) = 0;
    virtual void writeLong(long l) = 0;
    virtual void writeULong(unsigned long l) = 0;
    virtual void writeInt64(long long l) = 0;
    virtual void writeUInt64(unsigned long long l) = 0;
    virtual void writeFloat(float f) = 0;
    virtual void writeDouble(double d) = 0;
    virtual void writeString(const std::string& str) = 0;
    virtual void writeStream(std::ostream& (*fn)(std::ostream&)) = 0;
    virtual void writeBase(std::ios_base& (*fn)(std::ios_base&)) = 0;
    virtual void writeGLenum(const ObjectGLenum& value) = 0;
    virtual void writeProperty(const ObjectProperty& prop) = 0;
    virtual void writeMark(const ObjectMark& mark) = 0;
    virtual void writeCharArray(const char* s, unsigned int size) = 0;
    virtual void writeWrappedString(const std::string& str) = 0;

    void checkStream();

protected:
    std::ostream* _out;
    OutputStream* _outputStream;
    bool _supportBinaryBrackets;
    bool _failed;
};

class AsciiOutputIterator : public OutputIterator
{
public:
    explicit AsciiOutputIterator(std::ostream* output);

    virtual bool isBinary() const { return false; }

    virtual void writeBool(bool b);
    virtual void writeChar(char c);
    virtual void writeSChar(signed char c);
    virtual void writeUChar(unsigned char c);
    virtual void writeShort(short s);
    virtual void writeUShort(unsigned short s);
    virtual void writeInt(int i);
    virtual void writeUInt(unsigned int i);
    virtual void writeLong(long l);
    virtual void writeULong(unsigned long l);
    virtual void writeInt64(long long l);
    virtual void writeUInt64(unsigned long long l);
    virtual void writeFloat(float f);
    virtual void writeDouble(double d);
    virtual void writeString(const std::string& str);
    virtual void writeStream(std::ostream& (*fn)(std::ostream&));
    virtual void writeBase(std::ios_base& (*fn)(std::ios_base&));
    virtual void writeGLenum(const ObjectGLenum& value);
    virtual void writeProperty(const ObjectProperty& prop);
    virtual void writeMark(const ObjectMark& mark);
    virtual void writeCharArray(const char*, unsigned int) {}
    virtual void writeWrappedString(const std::string& str);

protected:
    inline void indentIfRequired()
    {
        if (_readyForIndent)
        {
            for (int i = 0; i < _indent; ++i) *_out << ' ';
            _readyForIndent = false;
        }
    }

    bool _readyForIndent;
    int _indent;
};

class BinaryOutputIterator : public OutputIterator
{
public:
    explicit BinaryOutputIterator(std::ostream* output);

    virtual bool isBinary() const { return true; }

    virtual void writeBool(bool b);
    virtual void writeChar(char c);
    virtual void writeSChar(signed char c);
    virtual void writeUChar(unsigned char c);
    virtual void writeShort(short s);
    virtual void writeUShort(unsigned short s);
    virtual void writeInt(int i);
    virtual void writeUInt(unsigned int i);
    virtual void writeLong(long l);
    virtual void writeULong(unsigned long l);
    virtual void writeInt64(long long l);
    virtual void writeUInt64(unsigned long long l);
    virtual void writeFloat(float f);
    virtual void writeDouble(double d);
    virtual void writeString(const std::string& str);
    virtual void writeStream(std::ostream& (*fn)(std::ostream&)) {}
    virtual void writeBase(std::ios_base& (*fn)(std::ios_base&)) {}
    virtual void writeGLenum(const ObjectGLenum& value);
    virtual void writeProperty(const ObjectProperty& prop);
    virtual void writeMark(const ObjectMark& mark);
    virtual void writeCharArray(const char* s, unsigned int size);
    virtual void writeWrappedString(const std::string& str);

protected:
    std::vector<std::streampos> _beginPositions;
};

// ---------------------------------------------------------------------------
// OutputUserData: the value source while writing.
//
// It holds the IR being written and exposes the commands recorded for each
// object (addChild / setMode / property assignments ...), so the ported
// `writeXxx` functions can fetch their data without knowing the file format.
// Property lookups also accept the legacy "set<Property>" spelling emitted by
// pre-112 readers (e.g. Geometry's "setVertexArray" for "VertexArray").
// ---------------------------------------------------------------------------
class OutputUserData
{
public:
    explicit OutputUserData(const SceneIR& ir);

    const SceneIR& getIR() const { return *_ir; }
    const std::vector<IRObject>& getObjects() const { return _ir->objects; }

    void setCurrentObject(int index) { _current = index; }
    int getCurrentObject() const { return _current; }
    const IRObject* currentObject() const;

    // Resolve an object reference (class name + id) to an IR object index.
    int resolve(const std::string& type, const std::string& id) const;
    int resolve(const IRArg& arg) const;

    // Commands of one object (defaults to the current one).
    const std::vector<const IRCommand*>& getCommands(int index = -1) const;

    // First command with this name (optionally requiring args[argIndex].i == value).
    const IRCommand* findCommand(const std::string& name, int index = -1) const;
    const IRCommand* findCommandWithInt(const std::string& name, long long value,
                                        unsigned int argIndex = 0, int index = -1) const;

    // All commands with this name, in read order.
    void findCommands(const std::string& name, std::vector<const IRCommand*>& out,
                      int index = -1) const;
    unsigned int countCommands(const std::string& name, int index = -1) const;

    // Property value lookup: exact name first, then the legacy "set<Name>" form.
    const IRArg* findPropertyArg(const std::string& name, unsigned int argIndex = 0,
                                 int index = -1) const;

    // Same, but only accepts an argument of a given kind.  Needed where two
    // wrappers use the same property name with different payloads (e.g.
    // Node::InitialBound is a bounding sphere while Drawable::InitialBound is
    // a bounding box, and both record a "setInitialBound" command).
    const IRArg* findPropertyArgOfKind(const std::string& name, IRArg::Kind kind,
                                       unsigned int argIndex = 0, int index = -1) const;

    void addWarning(const std::string& warning) const;
    const std::vector<std::string>& getWarnings() const { return _warnings; }

protected:
    void buildIndex();

    const SceneIR* _ir;
    int _current;
    std::map<int, std::vector<const IRCommand*> > _commands;
    mutable std::vector<std::string> _warnings;
};

// ---------------------------------------------------------------------------
// OutputStream
// ---------------------------------------------------------------------------
class OutputStream
{
public:
    enum WriteType { WRITE_UNKNOWN = 0, WRITE_SCENE = 1, WRITE_IMAGE = 2, WRITE_OBJECT = 3 };

    OutputStream();
    virtual ~OutputStream();

    void setOutputIterator(OutputIterator* out) { _out = out; }
    OutputIterator* getOutputIterator() const { return _out; }

    void setFileVersion(int version) { _targetFileVersion = version; }
    int getFileVersion(const std::string& domain = "") const;

    bool isBinary() const { return _out ? _out->isBinary() : false; }

    int getException() const { return _exception; }
    void setException(int e) { _exception = e; }
    void throwException(const std::string& msg);

    // Warnings collected while writing (missing wrapper / missing user writer).
    void setUserData(OutputUserData* ud) { _ud = ud; }
    void addWarning(const std::string& warning) const { if (_ud) _ud->addWarning(warning); }

    WriteType start(OutputIterator* out, WriteType type);

    ObjectProperty& PROPERTY(const std::string& name, int value = 0);

    unsigned int writeSize(unsigned int size) { *this << size; return size; }
    void writeWrappedString(const std::string& s) { if (_out) _out->writeWrappedString(s); }
    void writeCharArray(const char* s, unsigned int size) { if (_out) _out->writeCharArray(s, size); }

    // ---- IR-driven writing ----
    // Write the object at `index` (class name + UniqueID + wrapper fields), or a
    // reference-only record when the object was already written.  `osg::Image`
    // is routed to writeImage().
    void writeObject(OutputUserData& ud, int index);
    void writeImage(OutputUserData& ud, int index);

    // Write the array at `index` using the legacy compact encoding
    // (ArrayID + ArrayType + element data).  A handful of wrappers (e.g.
    // osgText::TextBase::_text, osgAnimation::MorphGeometry sources) read their
    // arrays through InputStream::readArray(), which expects that encoding.
    void writeCompactArray(OutputUserData& ud, int index);

    // Write the fields declared by a wrapper (associates first, then own fields).
    // Exposed for the serializer write dispatcher (GenericReserializer.cpp).
    void writeObjectFields(OutputUserData& ud, int index, class Rewrapper* wrapper);

    unsigned int findOrCreateObjectID(OutputUserData& ud, int index, bool& newID);

    ObjectMark BEGIN_BRACKET;
    ObjectMark END_BRACKET;

    // ---- operator<< primitives ----
    OutputStream& operator<<(bool b);
    OutputStream& operator<<(char c);
    OutputStream& operator<<(signed char c);
    OutputStream& operator<<(unsigned char c);
    OutputStream& operator<<(short s);
    OutputStream& operator<<(unsigned short s);
    OutputStream& operator<<(int i);
    OutputStream& operator<<(unsigned int i);
    OutputStream& operator<<(long l);
    OutputStream& operator<<(unsigned long l);
    OutputStream& operator<<(long long l);
    OutputStream& operator<<(unsigned long long l);
    OutputStream& operator<<(float f);
    OutputStream& operator<<(double d);
    OutputStream& operator<<(const char* str);
    OutputStream& operator<<(const std::string& str);
    OutputStream& operator<<(std::ostream& (*fn)(std::ostream&));
    OutputStream& operator<<(std::ios_base& (*fn)(std::ios_base&));
    OutputStream& operator<<(const ObjectGLenum& v);
    OutputStream& operator<<(const ObjectProperty& p);
    OutputStream& operator<<(const ObjectMark& m);

    template<unsigned int N, typename T>
    OutputStream& operator<<(const osg::Vec<N,T>& v)
    {
        for (unsigned int i = 0; i < N; ++i) *this << v._v[i];
        return *this;
    }

    OutputStream& operator<<(const osg::Quat& q);
    OutputStream& operator<<(const osg::Plane& p);
    OutputStream& operator<<(const osg::Matrixd& m);
    OutputStream& operator<<(const osg::Matrixf& m);
    OutputStream& operator<<(const osg::BoundingBoxf& bb);
    OutputStream& operator<<(const osg::BoundingBoxd& bb);
    OutputStream& operator<<(const osg::BoundingSpheref& bs);
    OutputStream& operator<<(const osg::BoundingSphered& bs);

protected:
    ObjectProperty _property;
    OutputIterator* _out;
    OutputUserData* _ud;
    int _targetFileVersion;
    int _exception;
    std::map<int, unsigned int> _objectIDs;   // IR object index -> written UniqueID
};

// Encode bytes as base64 (used for inline ASCII image data).
std::string encodeBase64(const unsigned char* data, size_t size);

// ---------------------------------------------------------------------------
// Small IRArg -> plain value helpers for the ported wrapper write functions.
// They never fail: missing components read as 0.
// ---------------------------------------------------------------------------
inline bool irArgBool(const IRArg& a)
{
    switch (a.kind)
    {
    case IRArg::BOOL: return a.b;
    case IRArg::INT: return a.i != 0;
    case IRArg::UINT: return a.u != 0;
    case IRArg::DOUBLE: return a.d != 0.0;
    default: return false;
    }
}

inline long long irArgInt(const IRArg& a, long long def = 0)
{
    switch (a.kind)
    {
    case IRArg::BOOL: return a.b ? 1 : 0;
    case IRArg::INT: return a.i;
    case IRArg::UINT: return (long long)a.u;
    case IRArg::DOUBLE: return (long long)a.d;
    default: return def;
    }
}

inline double irArgDouble(const IRArg& a, double def = 0.0)
{
    switch (a.kind)
    {
    case IRArg::BOOL: return a.b ? 1.0 : 0.0;
    case IRArg::INT: return (double)a.i;
    case IRArg::UINT: return (double)a.u;
    case IRArg::DOUBLE: return a.d;
    default: return def;
    }
}

inline std::string irArgString(const IRArg& a, const std::string& def = "")
{ return a.kind == IRArg::STRING ? a.s : def; }

inline osg::Vec3d irArgVec3d(const IRArg& a)
{
    osg::Vec3d v; v._v[0] = a.v[0]; v._v[1] = a.v[1]; v._v[2] = a.v[2]; return v;
}

inline osg::Vec3f irArgVec3f(const IRArg& a)
{
    osg::Vec3f v; v._v[0] = (float)a.v[0]; v._v[1] = (float)a.v[1];
    v._v[2] = (float)a.v[2]; return v;
}

inline osg::Vec4d irArgVec4d(const IRArg& a)
{
    osg::Vec4d v;
    for (int i = 0; i < 4; ++i) v._v[i] = a.v[i];
    return v;
}

inline osg::Vec4f irArgVec4f(const IRArg& a)
{
    osg::Vec4f v;
    for (int i = 0; i < 4; ++i) v._v[i] = (float)a.v[i];
    return v;
}

inline osg::Vec4i irArgVec4i(const IRArg& a)
{
    osg::Vec4i v;
    for (int i = 0; i < 4; ++i) v._v[i] = (int)a.v[i];
    return v;
}

inline osg::Quat irArgQuat(const IRArg& a)
{
    osg::Quat q;
    for (int i = 0; i < 4; ++i) q._v[i] = a.v[i];
    return q;
}

inline osg::BoundingSphered irArgBoundingSphere(const IRArg& a)
{
    osg::BoundingSphered bs;
    for (int i = 0; i < 3; ++i) bs._center._v[i] = a.v[i];
    bs._radius = a.v[3];
    return bs;
}

inline osg::BoundingBoxd irArgBoundingBox(const IRArg& a)
{
    osg::BoundingBoxd bb;
    for (int i = 0; i < 3; ++i) { bb._min._v[i] = a.v[i]; bb._max._v[i] = a.v[i + 3]; }
    return bb;
}

} // namespace osgVerse

#endif
