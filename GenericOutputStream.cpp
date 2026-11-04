#include "GenericOutputStream.h"
#include "GenericReserializer.h"

#include <cstring>
#include <climits>
#include <cstdio>
#include <typeinfo>

using namespace osgVerse;

// ---------------------------------------------------------------------------
// OutputIterator
// ---------------------------------------------------------------------------
OutputIterator::OutputIterator()
    : _out(NULL), _outputStream(NULL), _supportBinaryBrackets(false), _failed(false)
{}

OutputIterator::~OutputIterator() {}

void OutputIterator::checkStream()
{
    if (_out && _out->fail()) _failed = true;
}

// ---------------------------------------------------------------------------
// AsciiOutputIterator
// ---------------------------------------------------------------------------
AsciiOutputIterator::AsciiOutputIterator(std::ostream* output)
    : _readyForIndent(false), _indent(0)
{
    _out = output;
}

void AsciiOutputIterator::writeBool(bool b)
{ indentIfRequired(); *_out << (b ? "TRUE " : "FALSE "); }

void AsciiOutputIterator::writeChar(char c)
{ indentIfRequired(); *_out << (short)c << ' '; }

void AsciiOutputIterator::writeSChar(signed char c)
{ indentIfRequired(); *_out << (short)c << ' '; }

void AsciiOutputIterator::writeUChar(unsigned char c)
{ indentIfRequired(); *_out << (unsigned short)c << ' '; }

void AsciiOutputIterator::writeShort(short s)
{ indentIfRequired(); *_out << s << ' '; }

void AsciiOutputIterator::writeUShort(unsigned short s)
{ indentIfRequired(); *_out << s << ' '; }

void AsciiOutputIterator::writeInt(int i)
{ indentIfRequired(); *_out << i << ' '; }

void AsciiOutputIterator::writeUInt(unsigned int i)
{ indentIfRequired(); *_out << i << ' '; }

void AsciiOutputIterator::writeLong(long l)
{ indentIfRequired(); *_out << l << ' '; }

void AsciiOutputIterator::writeULong(unsigned long l)
{ indentIfRequired(); *_out << l << ' '; }

void AsciiOutputIterator::writeInt64(long long l)
{ indentIfRequired(); *_out << l << ' '; }

void AsciiOutputIterator::writeUInt64(unsigned long long l)
{ indentIfRequired(); *_out << l << ' '; }

void AsciiOutputIterator::writeFloat(float f)
{
    indentIfRequired();
    // Keep enough digits for a binary-exact round-trip of single precision.
    std::streamsize old = _out->precision();
    _out->precision(9);
    *_out << f << ' ';
    _out->precision(old);
}

void AsciiOutputIterator::writeDouble(double d)
{
    indentIfRequired();
    std::streamsize old = _out->precision();
    _out->precision(17);
    *_out << d << ' ';
    _out->precision(old);
}

void AsciiOutputIterator::writeString(const std::string& str)
{ indentIfRequired(); *_out << str << ' '; }

void AsciiOutputIterator::writeStream(std::ostream& (*fn)(std::ostream&))
{
    indentIfRequired();
    *_out << fn;
    if (fn == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
        _readyForIndent = true;
}

void AsciiOutputIterator::writeBase(std::ios_base& (*fn)(std::ios_base&))
{ indentIfRequired(); *_out << fn; }

void AsciiOutputIterator::writeGLenum(const ObjectGLenum& value)
{
    const std::string& name = UserLookupTableProxy::get("GL").value(value.get());
    std::string out = name.empty() ? std::to_string(value.get()) : name;
    // osg's ASCII files use the token without the "GL_" prefix (REPEAT,
    // MODULATE, ...); keep that spelling for compatibility.
    if (out.size() > 3 && out.compare(0, 3, "GL_") == 0) out = out.substr(3);
    indentIfRequired();
    *_out << out << ' ';
}

void AsciiOutputIterator::writeProperty(const ObjectProperty& prop)
{
    std::string out = prop._name;
    if (prop._mapProperty)
    {
        const std::string& name = UserLookupTableProxy::get(prop._name.c_str()).value(prop._value);
        out = name.empty() ? std::to_string(prop._value) : name;
    }
    indentIfRequired();
    *_out << out << ' ';
}

void AsciiOutputIterator::writeMark(const ObjectMark& mark)
{
    _indent += mark.getMark();
    indentIfRequired();
    *_out << mark.getName();
}

void AsciiOutputIterator::writeWrappedString(const std::string& str)
{
    std::string wrapped;
    wrapped.reserve(str.size() + 2);
    for (size_t i = 0; i < str.size(); ++i)
    {
        char ch = str[i];
        if (ch == '"' || ch == '\\') wrapped += '\\';
        wrapped += ch;
    }
    writeString("\"" + wrapped + "\"");
}

// ---------------------------------------------------------------------------
// BinaryOutputIterator
// ---------------------------------------------------------------------------
BinaryOutputIterator::BinaryOutputIterator(std::ostream* output)
{ _out = output; }

void BinaryOutputIterator::writeBool(bool b)
{ char c = b ? 1 : 0; _out->write(&c, CHAR_SIZE); }

void BinaryOutputIterator::writeChar(char c)
{ _out->write(&c, CHAR_SIZE); }

void BinaryOutputIterator::writeSChar(signed char c)
{ _out->write((const char*)&c, CHAR_SIZE); }

void BinaryOutputIterator::writeUChar(unsigned char c)
{ _out->write((const char*)&c, CHAR_SIZE); }

void BinaryOutputIterator::writeShort(short s)
{ _out->write((const char*)&s, SHORT_SIZE); }

void BinaryOutputIterator::writeUShort(unsigned short s)
{ _out->write((const char*)&s, SHORT_SIZE); }

void BinaryOutputIterator::writeInt(int i)
{ _out->write((const char*)&i, INT_SIZE); }

void BinaryOutputIterator::writeUInt(unsigned int i)
{ _out->write((const char*)&i, INT_SIZE); }

void BinaryOutputIterator::writeLong(long l)
{ int32_t value = (int32_t)l; _out->write((const char*)&value, LONG_SIZE); }

void BinaryOutputIterator::writeULong(unsigned long l)
{ uint32_t value = (uint32_t)l; _out->write((const char*)&value, LONG_SIZE); }

void BinaryOutputIterator::writeInt64(long long l)
{ _out->write((const char*)&l, INT64_SIZE); }

void BinaryOutputIterator::writeUInt64(unsigned long long l)
{ _out->write((const char*)&l, INT64_SIZE); }

void BinaryOutputIterator::writeFloat(float f)
{ _out->write((const char*)&f, FLOAT_SIZE); }

void BinaryOutputIterator::writeDouble(double d)
{ _out->write((const char*)&d, DOUBLE_SIZE); }

void BinaryOutputIterator::writeString(const std::string& str)
{
    int size = (int)str.size();
    _out->write((const char*)&size, INT_SIZE);
    if (size > 0) _out->write(str.c_str(), size);
}

void BinaryOutputIterator::writeGLenum(const ObjectGLenum& value)
{
    unsigned int e = value.get();
    _out->write((const char*)&e, GLENUM_SIZE);
}

void BinaryOutputIterator::writeProperty(const ObjectProperty& prop)
{
    if (prop._mapProperty)
    {
        int value = prop._value;
        _out->write((const char*)&value, INT_SIZE);
    }
}

void BinaryOutputIterator::writeMark(const ObjectMark& mark)
{
    if (!_supportBinaryBrackets) return;

    // osg version > 148 stores the block size on 8 bytes, otherwise 4.
    int version = (_outputStream ? _outputStream->getFileVersion() : 0);
    if (mark.getName() == "{")
    {
        _beginPositions.push_back(_out->tellp());
        if (version > 148)
        {
            uint64_t size = 0; _out->write((const char*)&size, INT64_SIZE);
        }
        else
        {
            int size = 0; _out->write((const char*)&size, INT_SIZE);
        }
    }
    else if (mark.getName() == "}" && !_beginPositions.empty())
    {
        std::streampos pos = _out->tellp();
        std::streampos beginPos = _beginPositions.back();
        _beginPositions.pop_back();
        _out->seekp(beginPos);

        std::streampos size64 = pos - beginPos;
        if (version > 148)
        {
            uint64_t size = (uint64_t)size64;
            _out->write((const char*)&size, INT64_SIZE);
        }
        else
        {
            int size = (int)size64;
            _out->write((const char*)&size, INT_SIZE);
        }
        _out->seekp(pos);
    }
    checkStream();
}

void BinaryOutputIterator::writeCharArray(const char* s, unsigned int size)
{ if (size > 0) _out->write(s, size); }

void BinaryOutputIterator::writeWrappedString(const std::string& str)
{ writeString(str); }

// ---------------------------------------------------------------------------
// OutputUserData
// ---------------------------------------------------------------------------
OutputUserData::OutputUserData(const SceneIR& ir)
    : _ir(&ir), _current(-1)
{
    buildIndex();
}

void OutputUserData::buildIndex()
{
    for (size_t i = 0; i < _ir->events.size(); ++i)
    {
        const IREvent& e = _ir->events[i];
        if (e.type != IREvent::APPLY_COMMAND || e.objectIndex < 0) continue;
        _commands[e.objectIndex].push_back(&e.cmd);
    }
}

const IRObject* OutputUserData::currentObject() const
{
    if (_current < 0 || _current >= (int)_ir->objects.size()) return NULL;
    return &_ir->objects[_current];
}

int OutputUserData::resolve(const std::string& type, const std::string& id) const
{
    return _ir->resolve(type, id);
}

int OutputUserData::resolve(const IRArg& arg) const
{
    if (arg.kind != IRArg::OBJECT || arg.objId.empty()) return -1;
    return _ir->resolve(arg.objType, arg.objId);
}

const std::vector<const IRCommand*>& OutputUserData::getCommands(int index) const
{
    if (index < 0) index = _current;
    static const std::vector<const IRCommand*> s_empty;
    std::map<int, std::vector<const IRCommand*> >::const_iterator itr = _commands.find(index);
    return itr != _commands.end() ? itr->second : s_empty;
}

const IRCommand* OutputUserData::findCommand(const std::string& name, int index) const
{
    const std::vector<const IRCommand*>& list = getCommands(index);
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i]->name == name) return list[i];
    return NULL;
}

const IRCommand* OutputUserData::findCommandWithInt(const std::string& name, long long value,
                                                    unsigned int argIndex, int index) const
{
    const std::vector<const IRCommand*>& list = getCommands(index);
    for (size_t i = 0; i < list.size(); ++i)
    {
        const IRCommand* cmd = list[i];
        if (cmd->name != name || cmd->args.size() <= argIndex) continue;
        const IRArg& a = cmd->args[argIndex];
        if ((a.kind == IRArg::INT && a.i == value) ||
            (a.kind == IRArg::UINT && (long long)a.u == value)) return cmd;
    }
    return NULL;
}

void OutputUserData::findCommands(const std::string& name,
                                  std::vector<const IRCommand*>& out, int index) const
{
    const std::vector<const IRCommand*>& list = getCommands(index);
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i]->name == name) out.push_back(list[i]);
}

unsigned int OutputUserData::countCommands(const std::string& name, int index) const
{
    unsigned int count = 0;
    const std::vector<const IRCommand*>& list = getCommands(index);
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i]->name == name) ++count;
    return count;
}

const IRArg* OutputUserData::findPropertyArg(const std::string& name, unsigned int argIndex,
                                             int index) const
{
    // Modern wrappers record the property name directly ("VertexArray",
    // "Binding", ...); pre-112 user serializers record the setter name
    // ("setVertexArray", "setBinding", ...).  Try both spellings.
    const char* candidates[2] = { name.c_str(), NULL };
    std::string setter;
    if (!name.empty())
    {
        setter = "set" + name;
        candidates[1] = setter.c_str();
    }

    for (int c = 0; c < 2; ++c)
    {
        const IRCommand* cmd = findCommand(candidates[c], index);
        if (cmd && cmd->args.size() > argIndex) return &cmd->args[argIndex];
    }
    return NULL;
}

const IRArg* OutputUserData::findPropertyArgOfKind(const std::string& name, IRArg::Kind kind,
                                                   unsigned int argIndex, int index) const
{
    const char* candidates[2] = { name.c_str(), NULL };
    std::string setter;
    if (!name.empty())
    {
        setter = "set" + name;
        candidates[1] = setter.c_str();
    }

    const std::vector<const IRCommand*>& list = getCommands(index);
    for (int c = 0; c < 2; ++c)
    {
        for (size_t i = 0; i < list.size(); ++i)
        {
            const IRCommand* cmd = list[i];
            if (cmd->name != candidates[c] || cmd->args.size() <= argIndex) continue;
            if (cmd->args[argIndex].kind == kind) return &cmd->args[argIndex];
        }
    }
    return NULL;
}

void OutputUserData::addWarning(const std::string& warning) const
{
    for (size_t i = 0; i < _warnings.size(); ++i)
        if (_warnings[i] == warning) return;
    _warnings.push_back(warning);
}

// ---------------------------------------------------------------------------
// OutputStream
// ---------------------------------------------------------------------------
OutputStream::OutputStream()
    : _out(NULL), _ud(NULL), _targetFileVersion(161), _exception(0)
{
    BEGIN_BRACKET.setName("{");
    BEGIN_BRACKET.setMark(+INDENT_VALUE);
    END_BRACKET.setName("}");
    END_BRACKET.setMark(-INDENT_VALUE);
}

OutputStream::~OutputStream() {}

int OutputStream::getFileVersion(const std::string& domain) const
{
    (void)domain;
    return _targetFileVersion;
}

void OutputStream::throwException(const std::string& msg)
{
    _exception = 1;
    std::cerr << "[OutputStream] " << msg << std::endl;
}

ObjectProperty& OutputStream::PROPERTY(const std::string& name, int value)
{
    _property.setName(name);
    _property.setValue(value);
    _property._mapProperty = false;
    return _property;
}

OutputStream::WriteType OutputStream::start(OutputIterator* out, WriteType type)
{
    _out = out;
    _objectIDs.clear();
    if (!_out)
    {
        throwException("OutputStream: null output iterator.");
        return WRITE_UNKNOWN;
    }
    _out->setOutputStream(this);

    if (isBinary())
    {
        *this << (unsigned int)type << (unsigned int)_targetFileVersion;

        // 0x4: robust binary brackets (block sizes), as written by osg >= 98.
        unsigned int attributes = 0x4;
        _out->setSupportBinaryBrackets(true);
        *this << attributes;
        *this << std::string("0");   // no compressor
    }
    else
    {
        std::string typeString = "Scene";
        if (type == WRITE_IMAGE) typeString = "Image";
        else if (type == WRITE_OBJECT) typeString = "Object";
        *this << typeString << std::endl;
        *this << PROPERTY("#Version") << (unsigned int)_targetFileVersion << std::endl;
        *this << PROPERTY("#Generator") << std::string("generic_wrappers")
              << std::string("1.0") << std::endl;
        *this << std::endl;
    }
    return type;
}

unsigned int OutputStream::findOrCreateObjectID(OutputUserData& ud, int index, bool& newID)
{
    (void)ud;
    std::map<int, unsigned int>::iterator itr = _objectIDs.find(index);
    if (itr != _objectIDs.end())
    {
        newID = false;
        return itr->second;
    }

    // osg numbers objects from 1 in write order; shared references then only
    // carry the id.  The ids of the source file are irrelevant: the IR may mix
    // separate id spaces (objects / compact arrays / images).
    unsigned int id = (unsigned int)_objectIDs.size() + 1;
    _objectIDs[index] = id;
    newID = true;
    return id;
}

void OutputStream::writeCompactArray(OutputUserData& ud, int index)
{
    setUserData(&ud);
    const std::vector<IRObject>& objs = ud.getObjects();
    if (index < 0 || index >= (int)objs.size()) return;

    const IRObject& obj = objs[index];
    int typeId = osgArrayTypeID(obj.className);
    if (typeId < 0)
    {
        addWarning("Cannot write compact array of unknown class " + obj.className);
        return;
    }

    int comps = obj.vectorComponents;
    if (comps < 1)
    {
        switch (typeId)
        {
        case ID_VEC2B_ARRAY: case ID_VEC2S_ARRAY: case ID_VEC2_ARRAY: case ID_VEC2D_ARRAY:
        case ID_VEC2UB_ARRAY: case ID_VEC2US_ARRAY: case ID_VEC2I_ARRAY: case ID_VEC2UI_ARRAY:
            comps = 2; break;
        case ID_VEC3B_ARRAY: case ID_VEC3S_ARRAY: case ID_VEC3_ARRAY: case ID_VEC3D_ARRAY:
        case ID_VEC3UB_ARRAY: case ID_VEC3US_ARRAY: case ID_VEC3I_ARRAY: case ID_VEC3UI_ARRAY:
            comps = 3; break;
        case ID_VEC4B_ARRAY: case ID_VEC4UB_ARRAY: case ID_VEC4S_ARRAY: case ID_VEC4_ARRAY:
        case ID_VEC4D_ARRAY: case ID_VEC4US_ARRAY: case ID_VEC4I_ARRAY: case ID_VEC4UI_ARRAY:
            comps = 4; break;
        default: comps = 1; break;
        }
    }

    *this << PROPERTY("ArrayID") << (unsigned int)(index + 1) << std::endl;
    *this << ObjectProperty("ArrayType", typeId, true) << std::endl;

    const std::vector<double>& values = obj.vectorData;
    unsigned int count = comps > 0 ? (unsigned int)(values.size() / (size_t)comps) : 0;
    *this << count << BEGIN_BRACKET << std::endl;

    const bool wide = (typeId == ID_DOUBLE_ARRAY || typeId == ID_VEC2D_ARRAY ||
                       typeId == ID_VEC3D_ARRAY || typeId == ID_VEC4D_ARRAY ||
                       typeId == ID_INT64_ARRAY || typeId == ID_UINT64_ARRAY);
    const bool narrow = (typeId == ID_BYTE_ARRAY || typeId == ID_UBYTE_ARRAY ||
                         typeId == ID_VEC2B_ARRAY || typeId == ID_VEC3B_ARRAY ||
                         typeId == ID_VEC4B_ARRAY || typeId == ID_VEC4UB_ARRAY ||
                         typeId == ID_VEC2UB_ARRAY || typeId == ID_VEC3UB_ARRAY ||
                         typeId == ID_VEC2S_ARRAY || typeId == ID_VEC3S_ARRAY ||
                         typeId == ID_VEC4S_ARRAY || typeId == ID_VEC2US_ARRAY ||
                         typeId == ID_VEC3US_ARRAY || typeId == ID_VEC4US_ARRAY);

    for (size_t i = 0; i < values.size(); ++i)
    {
        if (wide) *this << values[i];
        else if (narrow) *this << (short)values[i];
        else *this << (float)values[i];
        if (!isBinary()) *this << std::endl;
    }
    *this << END_BRACKET << std::endl;
}

void OutputStream::writeObject(OutputUserData& ud, int index)
{
    setUserData(&ud);
    if (index < 0 || index >= (int)ud.getObjects().size())
    {
        *this << std::string("NULL") << std::endl;
        return;
    }

    const IRObject& obj = ud.getObjects()[index];
    if (obj.className == "osg::Image")
    {
        writeImage(ud, index);
        return;
    }

    bool newID = false;
    unsigned int id = findOrCreateObjectID(ud, index, newID);

    *this << obj.className << BEGIN_BRACKET << std::endl;
    *this << PROPERTY("UniqueID") << id << std::endl;
    if (newID)
    {
        Rewrapper* wrapper = RewrapperManager::instance()->getRewrapper(obj.className);
        if (wrapper)
        {
            int saved = ud.getCurrentObject();
            writeObjectFields(ud, index, wrapper);
            ud.setCurrentObject(saved);
        }
        else ud.addWarning("No rewrapper registered for " + obj.className);
    }
    *this << END_BRACKET << std::endl;
}

void OutputStream::writeImage(OutputUserData& ud, int index)
{
    setUserData(&ud);
    if (index < 0 || index >= (int)ud.getObjects().size()) return;
    const IRObject& img = ud.getObjects()[index];

    bool newID = false;
    unsigned int id = findOrCreateObjectID(ud, index, newID);

    if (_targetFileVersion > 94)
        *this << PROPERTY("ClassName") << std::string("osg::Image") << std::endl;
    *this << PROPERTY("UniqueID") << id << std::endl;
    if (!newID) return;

    int decision = img.imageDecision;
    if (decision != IMAGE_INLINE_DATA && decision != IMAGE_INLINE_FILE &&
        decision != IMAGE_WRITE_OUT) decision = IMAGE_USE_FILE;

    *this << PROPERTY("FileName");
    _out->writeWrappedString(img.fileName);
    *this << std::endl;

    // The IR does not keep osg::Image::WriteHint; NO_PREFERENCE (0) is the
    // default and does not influence reading.
    *this << PROPERTY("WriteHint") << (int)0 << decision << std::endl;

    if (decision == IMAGE_INLINE_DATA)
    {
        int s = img.width > 0 ? img.width : 1;
        int t = img.height > 0 ? img.height : 1;
        int r = img.depth > 0 ? img.depth : 1;
        if (isBinary())
        {
            *this << img.origin << s << t << r << img.internalFormat;
            *this << img.pixelFormat << img.dataType << img.packing << img.allocationMode;
            writeSize((unsigned int)img.pixelData.size());
            if (!img.pixelData.empty())
                _out->writeCharArray((const char*)&img.pixelData[0],
                                     (unsigned int)img.pixelData.size());
            writeSize(0);   // no mipmaps
        }
        else
        {
            *this << PROPERTY("Origin") << img.origin << std::endl;
            *this << PROPERTY("Size") << s << t << r << std::endl;
            *this << PROPERTY("InternalTextureFormat") << img.internalFormat << std::endl;
            *this << PROPERTY("PixelFormat") << img.pixelFormat << std::endl;
            *this << PROPERTY("DataType") << img.dataType << std::endl;
            *this << PROPERTY("Packing") << img.packing << std::endl;
            *this << PROPERTY("AllocationMode") << img.allocationMode << std::endl;
            *this << PROPERTY("Data") << (unsigned int)1 << BEGIN_BRACKET << std::endl;
            _out->writeWrappedString(encodeBase64(img.pixelData.empty() ? NULL : &img.pixelData[0],
                                                  img.pixelData.size()));
            *this << END_BRACKET << std::endl;
        }
    }
    else if (decision == IMAGE_INLINE_FILE)
    {
        if (isBinary())
        {
            writeSize((unsigned int)img.pixelData.size());
            if (!img.pixelData.empty())
                _out->writeCharArray((const char*)&img.pixelData[0],
                                     (unsigned int)img.pixelData.size());
        }
        // ASCII inline-file images are not reconstructible from the IR (the
        // reader does not keep the embedded file payload); only the filename is
        // written, which degrades to an external reference.
    }

    Rewrapper* objectWrapper = RewrapperManager::instance()->getRewrapper("osg::Object");
    if (objectWrapper)
    {
        int saved = ud.getCurrentObject();
        writeObjectFields(ud, index, objectWrapper);
        ud.setCurrentObject(saved);
    }
    if (getException()) return;
}

void OutputStream::writeObjectFields(OutputUserData& ud, int index, Rewrapper* wrapper)
{
    if (!wrapper) return;
    const int version = getFileVersion();
    const std::vector<WrapperAssociate>& assocs = wrapper->getAssociates();
    for (size_t i = 0; i < assocs.size(); ++i)
    {
        const WrapperAssociate& a = assocs[i];
        if (a._firstVersion > version || version > a._lastVersion) continue;

        Rewrapper* r = RewrapperManager::instance()->getRewrapper(a._name);
        if (!r)
        {
            ud.addWarning("Associated wrapper not found: " + a._name);
            continue;
        }

        const std::vector<Rewrapper::SerializerPair>& sl = r->getSerializers();
        for (size_t j = 0; j < sl.size(); ++j)
        {
            BaseSerializer* s = sl[j].second.get();
            if (!s) continue;
            if (s->_firstVersion > version || version > s->_lastVersion) continue;

            ud.setCurrentObject(index);
            writeSerializerValue(*this, ud, sl[j].first, *s);
            if (getException()) return;
        }
    }
}

// ---- operator<< primitives ----
OutputStream& OutputStream::operator<<(bool b)
{ _out->writeBool(b); return *this; }
OutputStream& OutputStream::operator<<(char c)
{ _out->writeChar(c); return *this; }
OutputStream& OutputStream::operator<<(signed char c)
{ _out->writeSChar(c); return *this; }
OutputStream& OutputStream::operator<<(unsigned char c)
{ _out->writeUChar(c); return *this; }
OutputStream& OutputStream::operator<<(short s)
{ _out->writeShort(s); return *this; }
OutputStream& OutputStream::operator<<(unsigned short s)
{ _out->writeUShort(s); return *this; }
OutputStream& OutputStream::operator<<(int i)
{ _out->writeInt(i); return *this; }
OutputStream& OutputStream::operator<<(unsigned int i)
{ _out->writeUInt(i); return *this; }
OutputStream& OutputStream::operator<<(long l)
{ _out->writeLong(l); return *this; }
OutputStream& OutputStream::operator<<(unsigned long l)
{ _out->writeULong(l); return *this; }
OutputStream& OutputStream::operator<<(long long l)
{ _out->writeInt64(l); return *this; }
OutputStream& OutputStream::operator<<(unsigned long long l)
{ _out->writeUInt64(l); return *this; }
OutputStream& OutputStream::operator<<(float f)
{ _out->writeFloat(f); return *this; }
OutputStream& OutputStream::operator<<(double d)
{ _out->writeDouble(d); return *this; }
OutputStream& OutputStream::operator<<(const char* str)
{ _out->writeString(str ? std::string(str) : std::string()); return *this; }
OutputStream& OutputStream::operator<<(const std::string& str)
{ _out->writeString(str); return *this; }
OutputStream& OutputStream::operator<<(std::ostream& (*fn)(std::ostream&))
{ _out->writeStream(fn); return *this; }
OutputStream& OutputStream::operator<<(std::ios_base& (*fn)(std::ios_base&))
{ _out->writeBase(fn); return *this; }
OutputStream& OutputStream::operator<<(const ObjectGLenum& v)
{ _out->writeGLenum(v); return *this; }
OutputStream& OutputStream::operator<<(const ObjectProperty& p)
{ _out->writeProperty(p); return *this; }
OutputStream& OutputStream::operator<<(const ObjectMark& m)
{ _out->writeMark(m); return *this; }

// ---- operator<< math types ----
OutputStream& OutputStream::operator<<(const osg::Quat& q)
{
    *this << (double)q._v[0] << (double)q._v[1] << (double)q._v[2] << (double)q._v[3];
    return *this;
}

OutputStream& OutputStream::operator<<(const osg::Plane& p)
{
    *this << (double)p._v[0] << (double)p._v[1] << (double)p._v[2] << (double)p._v[3];
    return *this;
}

OutputStream& OutputStream::operator<<(const osg::Matrixd& m)
{
    *this << BEGIN_BRACKET << std::endl;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            *this << m._m[c * 4 + r];
    *this << END_BRACKET << std::endl;
    return *this;
}

OutputStream& OutputStream::operator<<(const osg::Matrixf& m)
{
    *this << BEGIN_BRACKET << std::endl;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            *this << (double)m._m[c * 4 + r];
    *this << END_BRACKET << std::endl;
    return *this;
}

OutputStream& OutputStream::operator<<(const osg::BoundingBoxf& bb)
{
    *this << bb._min._v[0] << bb._min._v[1] << bb._min._v[2]
          << bb._max._v[0] << bb._max._v[1] << bb._max._v[2];
    return *this;
}

OutputStream& OutputStream::operator<<(const osg::BoundingBoxd& bb)
{
    *this << bb._min._v[0] << bb._min._v[1] << bb._min._v[2]
          << bb._max._v[0] << bb._max._v[1] << bb._max._v[2];
    return *this;
}

OutputStream& OutputStream::operator<<(const osg::BoundingSpheref& bs)
{
    *this << bs._center._v[0] << bs._center._v[1] << bs._center._v[2] << bs._radius;
    return *this;
}

OutputStream& OutputStream::operator<<(const osg::BoundingSphered& bs)
{
    *this << bs._center._v[0] << bs._center._v[1] << bs._center._v[2] << bs._radius;
    return *this;
}

// ---------------------------------------------------------------------------
// Base64 (ASCII inline image data)
// ---------------------------------------------------------------------------
namespace osgVerse
{
std::string encodeBase64(const unsigned char* data, size_t size)
{
    static const char* s_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    if (!data || size == 0) return out;
    out.reserve(((size + 2) / 3) * 4);

    size_t i = 0;
    for (; i + 2 < size; i += 3)
    {
        unsigned int v = ((unsigned int)data[i] << 16) |
                         ((unsigned int)data[i + 1] << 8) | data[i + 2];
        out += s_chars[(v >> 18) & 0x3f];
        out += s_chars[(v >> 12) & 0x3f];
        out += s_chars[(v >> 6) & 0x3f];
        out += s_chars[v & 0x3f];
    }

    if (i < size)
    {
        unsigned int v = (unsigned int)data[i] << 16;
        if (i + 1 < size) v |= (unsigned int)data[i + 1] << 8;
        out += s_chars[(v >> 18) & 0x3f];
        out += s_chars[(v >> 12) & 0x3f];
        out += (i + 1 < size) ? s_chars[(v >> 6) & 0x3f] : '=';
        out += '=';
    }
    return out;
}
} // namespace osgVerse
