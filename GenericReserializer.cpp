#include "GenericReserializer.h"

#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <type_traits>

using namespace osgVerse;

const std::string SerializerVisitor::_emptyProperty = "";

void SerializerVisitor::traverse(WrappableSerializer& obj)
{
    if (_propertyList.empty() || !_manager) return;
    Rewrapper* r = _manager->getRewrapper(obj._type);
    if (r != NULL) r->accept(_manager, *this, _inputVersion, true);
}

void SerializerVisitor::applyValue(BaseSerializer& obj)
{
    std::type_index typeID(typeid(obj));
    SerializerMap::iterator it = _registry.find(typeID);
    if (it != _registry.end()) it->second(obj);
    // Unregistered value serializer types are silently ignored: an engine
    // registers only the types it handles.
}

void Rewrapper::splitAssociates(const std::string& src)
{
    std::string::size_type start = src.find_first_not_of(' ');
    while (start != std::string::npos)
    {
        std::string::size_type end = src.find_first_of(' ', start);
        if (end != std::string::npos)
        {
            _associates.push_back(WrapperAssociate(std::string(src, start, end - start)));
            start = src.find_first_not_of(' ', end);
        }
        else
        {
            _associates.push_back(WrapperAssociate(std::string(src, start, src.size() - start)));
            start = end;
        }
    }
}

void Rewrapper::add(const std::string& n, BaseSerializer* s)
{
    s->_firstVersion = _version;
    _serializers.push_back(SerializerPair(n, s));
}

void Rewrapper::clear()
{ _serializers.clear(); }

void Rewrapper::markSerializerAsRemoved(const std::string& name)
{
    for (std::vector<SerializerPair>::iterator itr = _serializers.begin();
         itr != _serializers.end(); ++itr)
    { if (itr->first == name) itr->second->_lastVersion = _version - 1; }
}

void Rewrapper::markAssociateAsRemoved(const std::string& name)
{
    for (std::vector<WrapperAssociate>::iterator itr = _associates.begin();
         itr != _associates.end(); ++itr)
    { if (itr->_name == name) {itr->_lastVersion = _version - 1; return;} }
}

void Rewrapper::markAssociateAsAdded(const std::string& name)
{
    for (std::vector<WrapperAssociate>::iterator itr = _associates.begin();
         itr != _associates.end(); ++itr)
    { if (itr->_name == name) {itr->_firstVersion = _version; return;} }
}

void Rewrapper::accept(RewrapperManager* manager, SerializerVisitor& v,
                       int inputVersion, bool includingAssociates)
{
    v.setManager(manager, inputVersion);
    if (!includingAssociates)
    {
        for (std::vector<SerializerPair>::iterator itr = _serializers.begin();
             itr != _serializers.end(); ++itr)
        {
            BaseSerializer* s = itr->second.get(); if (!s) continue;
            if (s->_firstVersion <= inputVersion &&
                inputVersion <= s->_lastVersion) s->accept(itr->first, v);
        }
        return;
    }

    for (std::vector<WrapperAssociate>::iterator itr = _associates.begin();
         itr != _associates.end(); ++itr)
    {
        if (itr->_firstVersion <= inputVersion && inputVersion <= itr->_lastVersion)
        {
            Rewrapper* r = manager->getRewrapper(itr->_name);
            if (r != NULL) r->accept(manager, v, inputVersion, false);
            else { std::cerr << "[Rewrapper] Associated wrapper " << itr->_name << " not found\n"; }
        }
    }
}

std::shared_ptr<RewrapperManager> RewrapperManager::instance()
{
    static std::shared_ptr<RewrapperManager> instance =
        std::make_shared<RewrapperManager>(); return instance;
}

RewrapperManager::~RewrapperManager()
{
}

void RewrapperManager::addRewrapper(const std::string& name, Rewrapper* r)
{
    RewrapperMap::iterator itr = _rewrappers.find(name); _rewrappers[name] = r;
}

void RewrapperManager::removeRewrapper(const std::string& name)
{
    RewrapperMap::iterator itr = _rewrappers.find(name);
    if (itr != _rewrappers.end()) { _rewrappers.erase(itr); }
}

Rewrapper* RewrapperManager::getRewrapper(const std::string& name)
{
    RewrapperMap::iterator itr = _rewrappers.find(name);
    if (itr != _rewrappers.end()) return itr->second.get(); else return NULL;
}

RegisterWrapperProxy::RegisterWrapperProxy(
        const std::string& name, const std::string& associates, AddPropFunc func)
{
    Rewrapper* r = new Rewrapper; r->setAssociates(associates); if (func) (*func)(*r);
    RewrapperManager::instance()->addRewrapper(name, r); _className = name;
}

RegisterWrapperProxy::~RegisterWrapperProxy()
{ RewrapperManager::instance()->removeRewrapper(_className); }

// ---------------------------------------------------------------------------
// readSerializerValue: read a value from the input stream according to the
// concrete serializer type.
// Before invocation (ASCII mode) the property-name token has already been
// consumed by matchString; binary mode files contain no property name and
// the value is read directly.
// ---------------------------------------------------------------------------
namespace
{
    template<typename T>
    bool tryReadValue(osgVerse::InputStream& is, osgVerse::BaseSerializer& s)
    {
        if (typeid(s) != typeid(osgVerse::ValueSerializer<T>)) return false;
        osgVerse::ValueSerializer<T>& vs = static_cast<osgVerse::ValueSerializer<T>&>(s);
        if (vs._hex) { is >> std::hex >> vs._v0 >> std::dec; }
        else { is >> vs._v0; }
        return true;
    }

    // Read a single element using the element-type token of the VectorSerializer.
    // The token may carry the "osgDB::BaseSerializer::" prefix (macro #TYPE
    // expansion); strip it here. Numeric components are appended to `out`
    // (component count returned), so an engine visitor can rebuild the array.
    int readVectorElement(osgVerse::InputStream& is, const std::string& rawType,
                          std::vector<double>* out, std::vector<std::string>* strs = NULL)
    {
        std::string t = rawType;
        std::string::size_type p = t.rfind("::");
        if (p != std::string::npos) t = t.substr(p + 2);

        if (t == "RW_BOOL") { bool v; is >> v; if (out) out->push_back(v ? 1.0 : 0.0); return 1; }
        else if (t == "RW_CHAR") { char v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_UCHAR") { unsigned char v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_SHORT") { short v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_USHORT") { unsigned short v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_INT") { int v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_UINT") { unsigned int v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_LONG") { long v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_ULONG") { unsigned long v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_FLOAT") { float v; is >> v; if (out) out->push_back((double)v); return 1; }
        else if (t == "RW_DOUBLE") { double v; is >> v; if (out) out->push_back(v); return 1; }
        else if (t == "RW_VEC2B") { osg::Vec2b v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back((double)v._v[i]); return 2; }
        else if (t == "RW_VEC2UB") { osg::Vec2ub v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back((double)v._v[i]); return 2; }
        else if (t == "RW_VEC2S") { osg::Vec2s v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back((double)v._v[i]); return 2; }
        else if (t == "RW_VEC2US") { osg::Vec2us v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back((double)v._v[i]); return 2; }
        else if (t == "RW_VEC2I") { osg::Vec2i v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back((double)v._v[i]); return 2; }
        else if (t == "RW_VEC2UI") { osg::Vec2ui v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back((double)v._v[i]); return 2; }
        else if (t == "RW_VEC2F") { osg::Vec2f v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back((double)v._v[i]); return 2; }
        else if (t == "RW_VEC2D") { osg::Vec2d v; is >> v; if (out) for (int i = 0; i < 2; ++i) out->push_back(v._v[i]); return 2; }
        else if (t == "RW_VEC3B") { osg::Vec3b v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back((double)v._v[i]); return 3; }
        else if (t == "RW_VEC3UB") { osg::Vec3ub v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back((double)v._v[i]); return 3; }
        else if (t == "RW_VEC3S") { osg::Vec3s v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back((double)v._v[i]); return 3; }
        else if (t == "RW_VEC3US") { osg::Vec3us v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back((double)v._v[i]); return 3; }
        else if (t == "RW_VEC3I") { osg::Vec3i v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back((double)v._v[i]); return 3; }
        else if (t == "RW_VEC3UI") { osg::Vec3ui v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back((double)v._v[i]); return 3; }
        else if (t == "RW_VEC3F") { osg::Vec3f v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back((double)v._v[i]); return 3; }
        else if (t == "RW_VEC3D") { osg::Vec3d v; is >> v; if (out) for (int i = 0; i < 3; ++i) out->push_back(v._v[i]); return 3; }
        else if (t == "RW_VEC4B") { osg::Vec4b v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back((double)v._v[i]); return 4; }
        else if (t == "RW_VEC4UB") { osg::Vec4ub v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back((double)v._v[i]); return 4; }
        else if (t == "RW_VEC4S") { osg::Vec4s v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back((double)v._v[i]); return 4; }
        else if (t == "RW_VEC4US") { osg::Vec4us v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back((double)v._v[i]); return 4; }
        else if (t == "RW_VEC4I") { osg::Vec4i v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back((double)v._v[i]); return 4; }
        else if (t == "RW_VEC4UI") { osg::Vec4ui v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back((double)v._v[i]); return 4; }
        else if (t == "RW_VEC4F") { osg::Vec4f v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back((double)v._v[i]); return 4; }
        else if (t == "RW_VEC4D") { osg::Vec4d v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back(v._v[i]); return 4; }
        else if (t == "RW_QUAT") { osg::Quat v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back(v._v[i]); return 4; }
        else if (t == "RW_PLANE") { osg::Plane v; is >> v; if (out) for (int i = 0; i < 4; ++i) out->push_back(v._v[i]); return 4; }
        else if (t == "RW_MATRIX" || t == "RW_MATRIXF") { osg::Matrixf v; is >> v; if (out) for (int i = 0; i < 16; ++i) out->push_back((double)v._m[i]); return 16; }
        else if (t == "RW_MATRIXD") { osg::Matrixd v; is >> v; if (out) for (int i = 0; i < 16; ++i) out->push_back(v._m[i]); return 16; }
        else if (t == "RW_BOUNDINGBOXF") { osg::BoundingBoxf v; is >> v; if (out) { for (int i = 0; i < 3; ++i) out->push_back((double)v._min._v[i]); for (int i = 0; i < 3; ++i) out->push_back((double)v._max._v[i]); } return 6; }
        else if (t == "RW_BOUNDINGBOXD" || t == "RW_BOUNDINGBOX") { osg::BoundingBoxd v; is >> v; if (out) { for (int i = 0; i < 3; ++i) out->push_back(v._min._v[i]); for (int i = 0; i < 3; ++i) out->push_back(v._max._v[i]); } return 6; }
        else if (t == "RW_BOUNDINGSPHEREF") { osg::BoundingSpheref v; is >> v; if (out) { for (int i = 0; i < 3; ++i) out->push_back((double)v._center._v[i]); out->push_back((double)v._radius); } return 4; }
        else if (t == "RW_BOUNDINGSPHERED" || t == "RW_BOUNDINGSPHERE") { osg::BoundingSphered v; is >> v; if (out) { for (int i = 0; i < 3; ++i) out->push_back(v._center._v[i]); out->push_back(v._radius); } return 4; }
        else if (t == "RW_STRING") { std::string v; is.readWrappedString(v); if (strs) strs->push_back(v); return 0; }
        else if (t == "RW_GLENUM") { osgVerse::ObjectGLenum v; is >> v; return 0; }
        else
        {
            std::cerr << "[readVectorElement] Unhandled vector element type: "
                      << rawType << std::endl;
            return -1;
        }
    }

    bool readVectorValue(osgVerse::InputStream& is, osgVerse::BaseSerializer& s)
    {
        osgVerse::VectorSerializer* vs = dynamic_cast<osgVerse::VectorSerializer*>(&s);
        if (!vs) return false;
        vs->_values.clear(); vs->_numComponents = 0;
        vs->_stringValues.clear();

        unsigned int size = 0;
        is >> size;
        if (size > 0)
        {
            // Standard osg binary: VectorSerializer has no brackets (size + elements)
            if (!is.isBinary()) is >> is.BEGIN_BRACKET;
            for (unsigned int i = 0; i < size; ++i)
            {
                int n = readVectorElement(is, vs->_type, &vs->_values, &vs->_stringValues);
                if (n > 0) vs->_numComponents = n;
                if (is.getException()) return true;
            }
            if (!is.isBinary()) is >> is.END_BRACKET;
        }
        return true;
    }

    template<typename T>
    bool tryReadList(osgVerse::InputStream& is, osgVerse::BaseSerializer& s)
    {
        if (typeid(s) != typeid(osgVerse::ListSerializer<T>)) return false;
        osgVerse::ListSerializer<T>& ls = static_cast<osgVerse::ListSerializer<T>&>(s);
        unsigned int size = 0; is >> size;
        if (size > 0)
        {
            // Standard osg binary: ListSerializer has no brackets (size + elements)
            if (!is.isBinary()) is >> is.BEGIN_BRACKET;
            for (unsigned int i = 0; i < size; ++i)
            {
                T v; is >> v;
                if (i == 0) ls._v0 = v;
                if (is.getException()) return true;
            }
            if (!is.isBinary()) is >> is.END_BRACKET;
        }
        return true;
    }

    bool readObjectListValue(osgVerse::InputStream& is, osgVerse::InputUserData& ud,
                             osgVerse::ListSerializer<osgVerse::ObjectTypeAndID>& ls)
    {
        ls._elements.clear();
        unsigned int size = 0; is >> size;
        if (size > 0)
        {
            // Standard osg binary: ListSerializer has no brackets (size + element objects);
            // only ASCII mode writes BEGIN_BRACKET/END_BRACKET
            if (!is.isBinary()) is >> is.BEGIN_BRACKET;
            for (unsigned int i = 0; i < size; ++i)
            {
                osgVerse::ObjectTypeAndID oid;
                // Since osg 112 the writer stores arrays and primitive sets as
                // full objects regardless of ASCII/binary; older files use the
                // compact forms (see InputUserData::readObjectFromStream).
                if (is.getFileVersion() >= 112)
                    oid = is.readObject(ud);
                else if (ls._elemType == "osg::PrimitiveSet")
                    oid = is.readPrimitiveSet(ud);
                else if (ls._elemType == "osg::Array")
                    oid = is.readArray(ud);
                else
                    oid = is.readObject(ud);
                if (i == 0) ls._v0 = oid;
                ls._elements.push_back(oid);
                if (is.getException()) return true;
            }
            if (!is.isBinary()) is >> is.END_BRACKET;
        }
        return true;
    }
}

namespace osgVerse
{
    bool readSerializerValue(InputStream& is, InputUserData& ud, BaseSerializer& s)
    {
        // Bool
        if (BoolSerializer* bs = dynamic_cast<BoolSerializer*>(&s))
        { is >> bs->_v0; return true; }

        // String
        if (StringSerializer* ss = dynamic_cast<StringSerializer*>(&s))
        { is.readWrappedString(ss->_v0); return true; }

        // Enum: ASCII uses a name token, binary uses an integer (stored as a string)
        if (EnumSerializer* es = dynamic_cast<EnumSerializer*>(&s))
        {
            if (is.isBinary())
            {
                unsigned int v = 0; is >> v; es->_v0 = std::to_string(v);
            }
            else is >> es->_v0;
            return true;
        }

        // GLenum
        if (GLenumSerializer* gs = dynamic_cast<GLenumSerializer*>(&s))
        {
            if (is.isBinary()) { is >> gs->_v0; return true; }
            ObjectGLenum gl; is >> gl; gs->_v0 = gl.get(); return true;
        }

        // BitFlag
        if (BitFlagSerializer* bfs = dynamic_cast<BitFlagSerializer*>(&s))
        {
            if (is.isBinary()) { is >> bfs->_v0; return true; }
            if (bfs->_asInt) { int v = 0; is >> v; bfs->_v0 = (unsigned int)v; }
            else { is >> bfs->_v0; }
            return true;
        }

        // User serializer: in binary mode read the ok flag first (the writer decides
        // whether to emit it via the check function; the reader reads it back,
        // and false means the field content is skipped); in ASCII mode the
        // reader is invoked directly.
        if (UserSerializer* us = dynamic_cast<UserSerializer*>(&s))
        {
            if (is.isBinary())
            {
                bool ok = false;
                is >> ok;
                if (!ok) return true;
            }
            if (us->_reader) us->_reader(is, ud);
            return true;
        }

        // Object serializer: read hasObject + nested object.
        // The 1-byte hasObject flag belongs to THIS field's protocol (like the
        // ImageSerializer above), so it must be consumed here, not inside
        // readObjectFromStream - user-serializer functions calling the latter
        // directly (e.g. Geometry's VertexData) have their own bool flags.
        if (ObjectSerializer* os_ = dynamic_cast<ObjectSerializer*>(&s))
        {
            os_->_readValue = ObjectTypeAndID();
            if (is.isBinary())
            {
                bool hasObject = false;
                is >> hasObject;
                if (hasObject) os_->_readValue = is.readObject(ud);
            }
            else
            {
                // ASCII also carries the has-object flag (e.g. "StateSet TRUE { ... }").
                bool hasObject = false; is >> hasObject;
                if (hasObject)
                {
                    is >> is.BEGIN_BRACKET;
                    os_->_readValue = ud.readObjectFromStream(is, os_->_type);
                    is >> is.END_BRACKET;
                }
            }
            return true;
        }

        // Image serializer
        if (ImageSerializer* is_ = dynamic_cast<ImageSerializer*>(&s))
        {
            is_->_readValue = ObjectTypeAndID();
            if (is.isBinary())
            {
                bool hasObject = false; is >> hasObject;
                if (hasObject) is_->_readValue = is.readImage(ud);
            }
            else
            {
                bool hasObject = false; is >> hasObject;
                if (hasObject)
                {
                    is >> is.BEGIN_BRACKET;
                    is_->_readValue = is.readImage(ud);
                    is >> is.END_BRACKET;
                }
            }
            return true;
        }

        // Object list (PrimitiveSetList / ClipPlaneList etc.)
        if (ListSerializer<ObjectTypeAndID>* ls = dynamic_cast<ListSerializer<ObjectTypeAndID>*>(&s))
        { return readObjectListValue(is, ud, *ls); }

        // VectorSerializer: the data container for an Array's vector (e.g. Vec3fArray)
        if (readVectorValue(is, s)) return true;

        // Primitive value lists (TimeList / ValueList etc.)
        if (tryReadList<double>(is, s)) return true;
        if (tryReadList<float>(is, s)) return true;
        if (tryReadList<bool>(is, s)) return true;
        if (tryReadList<int>(is, s)) return true;
        if (tryReadList<unsigned int>(is, s)) return true;

        // ValueSerializer<T>: scalar
        if (tryReadValue<char>(is, s)) return true;
        if (tryReadValue<signed char>(is, s)) return true;
        if (tryReadValue<unsigned char>(is, s)) return true;
        if (tryReadValue<short>(is, s)) return true;
        if (tryReadValue<unsigned short>(is, s)) return true;
        if (tryReadValue<int>(is, s)) return true;
        if (tryReadValue<unsigned int>(is, s)) return true;
        if (tryReadValue<long>(is, s)) return true;
        if (tryReadValue<unsigned long>(is, s)) return true;
        if (tryReadValue<float>(is, s)) return true;
        if (tryReadValue<double>(is, s)) return true;

        // ValueSerializer<T>: vector / math type
        if (tryReadValue<osg::Vec2b>(is, s)) return true;
        if (tryReadValue<osg::Vec2ub>(is, s)) return true;
        if (tryReadValue<osg::Vec2s>(is, s)) return true;
        if (tryReadValue<osg::Vec2us>(is, s)) return true;
        if (tryReadValue<osg::Vec2i>(is, s)) return true;
        if (tryReadValue<osg::Vec2ui>(is, s)) return true;
        if (tryReadValue<osg::Vec2f>(is, s)) return true;
        if (tryReadValue<osg::Vec2d>(is, s)) return true;
        if (tryReadValue<osg::Vec3b>(is, s)) return true;
        if (tryReadValue<osg::Vec3ub>(is, s)) return true;
        if (tryReadValue<osg::Vec3s>(is, s)) return true;
        if (tryReadValue<osg::Vec3us>(is, s)) return true;
        if (tryReadValue<osg::Vec3i>(is, s)) return true;
        if (tryReadValue<osg::Vec3ui>(is, s)) return true;
        if (tryReadValue<osg::Vec3f>(is, s)) return true;
        if (tryReadValue<osg::Vec3d>(is, s)) return true;
        if (tryReadValue<osg::Vec4b>(is, s)) return true;
        if (tryReadValue<osg::Vec4ub>(is, s)) return true;
        if (tryReadValue<osg::Vec4s>(is, s)) return true;
        if (tryReadValue<osg::Vec4us>(is, s)) return true;
        if (tryReadValue<osg::Vec4i>(is, s)) return true;
        if (tryReadValue<osg::Vec4ui>(is, s)) return true;
        if (tryReadValue<osg::Vec4f>(is, s)) return true;
        if (tryReadValue<osg::Vec4d>(is, s)) return true;
        if (tryReadValue<osg::Quat>(is, s)) return true;
        if (tryReadValue<osg::Plane>(is, s)) return true;
        if (tryReadValue<osg::Matrix>(is, s)) return true;
        if (tryReadValue<osg::Matrixf>(is, s)) return true;
        if (tryReadValue<osg::Matrixd>(is, s)) return true;
        if (tryReadValue<osg::BoundingBoxf>(is, s)) return true;
        if (tryReadValue<osg::BoundingBoxd>(is, s)) return true;
        if (tryReadValue<osg::BoundingSpheref>(is, s)) return true;
        if (tryReadValue<osg::BoundingSphered>(is, s)) return true;

        std::cerr << "[readSerializerValue] Unhandled serializer type: "
                  << typeid(s).name() << std::endl;
        return false;
    }
}

// ===========================================================================
// Write side
//
// The write dispatcher mirrors readSerializerValue: it emits exactly the layout
// the reader expects for each serializer kind.  Values are looked up in the
// SceneIR commands of the object currently being written (see OutputUserData);
// when a command is absent the serializer's declared default is used, so a
// partial IR still produces a well-formed file.
// ===========================================================================
namespace
{
    // ---- scalar IR value helpers ----
    bool irToBool(const IRArg* a, bool def)
    {
        if (!a) return def;
        switch (a->kind)
        {
        case IRArg::BOOL: return a->b;
        case IRArg::INT: return a->i != 0;
        case IRArg::UINT: return a->u != 0;
        case IRArg::DOUBLE: return a->d != 0.0;
        case IRArg::STRING: return a->s == "TRUE" || a->s == "true" || a->s == "1";
        default: return def;
        }
    }

    long long irToInt(const IRArg* a, long long def)
    {
        if (!a) return def;
        switch (a->kind)
        {
        case IRArg::BOOL: return a->b ? 1 : 0;
        case IRArg::INT: return a->i;
        case IRArg::UINT: return (long long)a->u;
        case IRArg::DOUBLE: return (long long)a->d;
        case IRArg::STRING: return a->s.empty() ? def : (long long)strtoll(a->s.c_str(), NULL, 0);
        default: return def;
        }
    }

    unsigned long long irToUInt(const IRArg* a, unsigned long long def)
    {
        if (!a) return def;
        switch (a->kind)
        {
        case IRArg::BOOL: return a->b ? 1 : 0;
        case IRArg::INT: return (unsigned long long)a->i;
        case IRArg::UINT: return a->u;
        case IRArg::DOUBLE: return (unsigned long long)a->d;
        case IRArg::STRING:
            return a->s.empty() ? def : (unsigned long long)strtoull(a->s.c_str(), NULL, 0);
        default: return def;
        }
    }

    double irToDouble(const IRArg* a, double def)
    {
        if (!a) return def;
        switch (a->kind)
        {
        case IRArg::BOOL: return a->b ? 1.0 : 0.0;
        case IRArg::INT: return (double)a->i;
        case IRArg::UINT: return (double)a->u;
        case IRArg::DOUBLE: return a->d;
        case IRArg::STRING: return a->s.empty() ? def : atof(a->s.c_str());
        default: return def;
        }
    }

    std::string irToString(const IRArg* a, const std::string& def)
    {
        if (!a) return def;
        switch (a->kind)
        {
        case IRArg::STRING: return a->s;
        case IRArg::BOOL: return a->b ? "TRUE" : "FALSE";
        case IRArg::INT: return std::to_string(a->i);
        case IRArg::UINT: return std::to_string(a->u);
        case IRArg::DOUBLE: { std::ostringstream ss; ss << a->d; return ss.str(); }
        default: return def;
        }
    }

    // ---- IR value -> typed value ----
    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
    bool assignFromArg(const IRArg& a, T& out)
    {
        switch (a.kind)
        {
        case IRArg::BOOL: out = (T)(a.b ? 1 : 0); return true;
        case IRArg::INT: out = (T)a.i; return true;
        case IRArg::UINT: out = (T)a.u; return true;
        case IRArg::DOUBLE: out = (T)a.d; return true;
        case IRArg::STRING:
            if (a.s.empty()) return false;
            { std::istringstream iss(a.s); iss >> out; return !iss.fail(); }
        default: return false;
        }
    }

    template<unsigned int N, typename T>
    bool assignFromArg(const IRArg& a, osg::Vec<N, T>& out)
    {
        if (a.n < (int)N) return false;
        for (unsigned int i = 0; i < N; ++i) out._v[i] = (T)a.v[i];
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::Quat& out)
    {
        if (a.n < 4) return false;
        for (int i = 0; i < 4; ++i) out._v[i] = a.v[i];
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::Plane& out)
    {
        if (a.n < 4) return false;
        for (int i = 0; i < 4; ++i) out._v[i] = a.v[i];
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::Matrixd& out)
    {
        if (a.n < 16) return false;
        for (int i = 0; i < 16; ++i) out._m[i] = a.v[i];
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::Matrixf& out)
    {
        if (a.n < 16) return false;
        for (int i = 0; i < 16; ++i) out._m[i] = (float)a.v[i];
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::BoundingBoxf& out)
    {
        if (a.n < 6) return false;
        for (int i = 0; i < 3; ++i)
        { out._min._v[i] = (float)a.v[i]; out._max._v[i] = (float)a.v[i + 3]; }
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::BoundingBoxd& out)
    {
        if (a.n < 6) return false;
        for (int i = 0; i < 3; ++i)
        { out._min._v[i] = a.v[i]; out._max._v[i] = a.v[i + 3]; }
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::BoundingSpheref& out)
    {
        if (a.n < 4) return false;
        for (int i = 0; i < 3; ++i) out._center._v[i] = (float)a.v[i];
        out._radius = (float)a.v[3];
        return true;
    }

    bool assignFromArg(const IRArg& a, osg::BoundingSphered& out)
    {
        if (a.n < 4) return false;
        for (int i = 0; i < 3; ++i) out._center._v[i] = a.v[i];
        out._radius = a.v[3];
        return true;
    }

    // Strip "osgDB::BaseSerializer::" and similar prefixes from a RW_ token.
    std::string normalizedToken(const std::string& raw)
    {
        std::string t = raw;
        std::string::size_type p = t.rfind("::");
        if (p != std::string::npos) t = t.substr(p + 2);
        return t;
    }

    int tokenComponents(const std::string& token)
    {
        if (token.find("RW_VEC2") != std::string::npos) return 2;
        if (token.find("RW_VEC3") != std::string::npos) return 3;
        if (token.find("RW_VEC4") != std::string::npos) return 4;
        if (token == "RW_QUAT" || token == "RW_PLANE" || token == "RW_BOUNDINGSPHEREF" ||
            token == "RW_BOUNDINGSPHERED" || token == "RW_BOUNDINGSPHERE") return 4;
        if (token == "RW_BOUNDINGBOXF" || token == "RW_BOUNDINGBOXD" ||
            token == "RW_BOUNDINGBOX") return 6;
        if (token == "RW_MATRIX" || token == "RW_MATRIXF" || token == "RW_MATRIXD") return 16;
        return 1;
    }

    // Write one vector element of the given RW_ token from `values[offset..]`.
    bool writeVectorElement(OutputStream& os, const std::string& token,
                            const std::vector<double>& values, size_t offset, int components)
    {
        if (offset + (size_t)components > values.size()) return false;

#define WRITE_SCALAR_TOKEN(TOKEN, TYPE) \
        if (token == TOKEN) { os << (TYPE)values[offset]; return true; }
        WRITE_SCALAR_TOKEN("RW_BOOL", bool)
        WRITE_SCALAR_TOKEN("RW_CHAR", char)
        WRITE_SCALAR_TOKEN("RW_UCHAR", unsigned char)
        WRITE_SCALAR_TOKEN("RW_SHORT", short)
        WRITE_SCALAR_TOKEN("RW_USHORT", unsigned short)
        WRITE_SCALAR_TOKEN("RW_INT", int)
        WRITE_SCALAR_TOKEN("RW_UINT", unsigned int)
        WRITE_SCALAR_TOKEN("RW_LONG", long)
        WRITE_SCALAR_TOKEN("RW_ULONG", unsigned long)
        WRITE_SCALAR_TOKEN("RW_FLOAT", float)
        WRITE_SCALAR_TOKEN("RW_DOUBLE", double)
#undef WRITE_SCALAR_TOKEN

#define WRITE_VEC_TOKEN(TOKEN, TYPE) \
        if (token == TOKEN) { \
            osg::TYPE v; \
            for (int i = 0; i < components; ++i) v._v[i] = values[offset + i]; \
            os << v; return true; }
        WRITE_VEC_TOKEN("RW_VEC2B", Vec2b)   WRITE_VEC_TOKEN("RW_VEC2UB", Vec2ub)
        WRITE_VEC_TOKEN("RW_VEC2S", Vec2s)   WRITE_VEC_TOKEN("RW_VEC2US", Vec2us)
        WRITE_VEC_TOKEN("RW_VEC2I", Vec2i)   WRITE_VEC_TOKEN("RW_VEC2UI", Vec2ui)
        WRITE_VEC_TOKEN("RW_VEC2F", Vec2f)   WRITE_VEC_TOKEN("RW_VEC2D", Vec2d)
        WRITE_VEC_TOKEN("RW_VEC3B", Vec3b)   WRITE_VEC_TOKEN("RW_VEC3UB", Vec3ub)
        WRITE_VEC_TOKEN("RW_VEC3S", Vec3s)   WRITE_VEC_TOKEN("RW_VEC3US", Vec3us)
        WRITE_VEC_TOKEN("RW_VEC3I", Vec3i)   WRITE_VEC_TOKEN("RW_VEC3UI", Vec3ui)
        WRITE_VEC_TOKEN("RW_VEC3F", Vec3f)   WRITE_VEC_TOKEN("RW_VEC3D", Vec3d)
        WRITE_VEC_TOKEN("RW_VEC4B", Vec4b)   WRITE_VEC_TOKEN("RW_VEC4UB", Vec4ub)
        WRITE_VEC_TOKEN("RW_VEC4S", Vec4s)   WRITE_VEC_TOKEN("RW_VEC4US", Vec4us)
        WRITE_VEC_TOKEN("RW_VEC4I", Vec4i)   WRITE_VEC_TOKEN("RW_VEC4UI", Vec4ui)
        WRITE_VEC_TOKEN("RW_VEC4F", Vec4f)   WRITE_VEC_TOKEN("RW_VEC4D", Vec4d)
#undef WRITE_VEC_TOKEN

        if (token == "RW_QUAT")
        { osg::Quat v; for (int i = 0; i < 4; ++i) v._v[i] = values[offset + i]; os << v; return true; }
        if (token == "RW_PLANE")
        { osg::Plane v; for (int i = 0; i < 4; ++i) v._v[i] = values[offset + i]; os << v; return true; }
        if (token == "RW_MATRIX" || token == "RW_MATRIXF")
        { osg::Matrixf v; for (int i = 0; i < 16; ++i) v._m[i] = (float)values[offset + i]; os << v; return true; }
        if (token == "RW_MATRIXD")
        { osg::Matrixd v; for (int i = 0; i < 16; ++i) v._m[i] = values[offset + i]; os << v; return true; }
        if (token == "RW_BOUNDINGBOXF")
        {
            osg::BoundingBoxf v;
            for (int i = 0; i < 3; ++i) { v._min._v[i] = (float)values[offset + i];
                                          v._max._v[i] = (float)values[offset + 3 + i]; }
            os << v; return true;
        }
        if (token == "RW_BOUNDINGBOXD" || token == "RW_BOUNDINGBOX")
        {
            osg::BoundingBoxd v;
            for (int i = 0; i < 3; ++i) { v._min._v[i] = values[offset + i];
                                          v._max._v[i] = values[offset + 3 + i]; }
            os << v; return true;
        }
        if (token == "RW_BOUNDINGSPHEREF")
        {
            osg::BoundingSpheref v;
            for (int i = 0; i < 3; ++i) v._center._v[i] = (float)values[offset + i];
            v._radius = (float)values[offset + 3]; os << v; return true;
        }
        if (token == "RW_BOUNDINGSPHERED" || token == "RW_BOUNDINGSPHERE")
        {
            osg::BoundingSphered v;
            for (int i = 0; i < 3; ++i) v._center._v[i] = values[offset + i];
            v._radius = values[offset + 3]; os << v; return true;
        }
        return false;
    }

    bool supportedVectorToken(const std::string& token)
    {
        static const char* s_tokens[] = {
            "RW_BOOL", "RW_CHAR", "RW_UCHAR", "RW_SHORT", "RW_USHORT", "RW_INT",
            "RW_UINT", "RW_LONG", "RW_ULONG", "RW_FLOAT", "RW_DOUBLE",
            "RW_VEC2B", "RW_VEC2UB", "RW_VEC2S", "RW_VEC2US", "RW_VEC2I", "RW_VEC2UI",
            "RW_VEC2F", "RW_VEC2D",
            "RW_VEC3B", "RW_VEC3UB", "RW_VEC3S", "RW_VEC3US", "RW_VEC3I", "RW_VEC3UI",
            "RW_VEC3F", "RW_VEC3D",
            "RW_VEC4B", "RW_VEC4UB", "RW_VEC4S", "RW_VEC4US", "RW_VEC4I", "RW_VEC4UI",
            "RW_VEC4F", "RW_VEC4D",
            "RW_QUAT", "RW_PLANE", "RW_MATRIX", "RW_MATRIXF", "RW_MATRIXD",
            "RW_BOUNDINGBOXF", "RW_BOUNDINGBOXD", "RW_BOUNDINGBOX",
            "RW_BOUNDINGSPHEREF", "RW_BOUNDINGSPHERED", "RW_BOUNDINGSPHERE"
        };
        for (size_t i = 0; i < sizeof(s_tokens) / sizeof(s_tokens[0]); ++i)
            if (token == s_tokens[i]) return true;
        return false;
    }

    // size [ { element... } ]  (binary: no brackets; ASCII: brackets when size>0)
    bool writeVectorValues(OutputStream& os, const std::string& rawToken,
                           const std::vector<double>& values, int components,
                           const std::vector<std::string>* strings = NULL)
    {
        std::string token = normalizedToken(rawToken);
        if (components <= 0) components = tokenComponents(token);

        if (token == "RW_STRING")
        {
            unsigned int size = strings ? (unsigned int)strings->size() : 0;
            os << size;
            if (size > 0 && !os.isBinary()) os << os.BEGIN_BRACKET << std::endl;
            for (unsigned int i = 0; i < size; ++i)
            {
                os.writeWrappedString((*strings)[i]);
                if (!os.isBinary()) os << std::endl;
            }
            if (!os.isBinary())
            {
                if (size > 0) os << os.END_BRACKET << std::endl;
                else os << std::endl;
            }
            return true;
        }

        unsigned int size = 0;
        if (supportedVectorToken(token) && components > 0)
            size = (unsigned int)(values.size() / (size_t)components);
        else
            os.addWarning("Unhandled vector element type: " + rawToken);

        os << size;
        if (size > 0 && !os.isBinary()) os << os.BEGIN_BRACKET << std::endl;

        for (unsigned int i = 0; i < size; ++i)
        {
            if (!writeVectorElement(os, token, values, (size_t)i * components, components)) break;
            if (!os.isBinary()) os << std::endl;
        }

        if (!os.isBinary())
        {
            if (size > 0) os << os.END_BRACKET << std::endl;
            else os << std::endl;
        }
        return true;
    }

    // Indexed legacy commands ("setTexCoordArray i obj") -> ordered objects.
    void collectIndexedObjects(OutputUserData& ud, const std::string& setter,
                               std::vector<int>& targets)
    {
        struct Entry { long long index; int target; };
        std::vector<Entry> entries;

        const std::vector<const IRCommand*>& list = ud.getCommands();
        for (size_t i = 0; i < list.size(); ++i)
        {
            const IRCommand* cmd = list[i];
            if (cmd->name != setter || cmd->args.size() < 2) continue;
            Entry e;
            e.index = irToInt(&cmd->args[0], 0);
            e.target = ud.resolve(cmd->args[1]);
            entries.push_back(e);
        }
        std::sort(entries.begin(), entries.end(),
                  [](const Entry& a, const Entry& b) { return a.index < b.index; });
        for (size_t i = 0; i < entries.size(); ++i) targets.push_back(entries[i].target);
    }

    bool isNumericString(const std::string& s)
    {
        if (s.empty()) return false;
        char* end = NULL;
        strtoll(s.c_str(), &end, 0);
        return end && *end == '\0';
    }

    // ASCII enum tokens are kept verbatim.  Binary enums are integers; when the
    // IR only knows the token (ASCII source) we cannot invert the enum lookup
    // table yet, so emit 0 and report it (round-tripping the same format, or
    // binary->binary / ASCII->ASCII, is unaffected).
    unsigned int enumTokenToValue(OutputStream& os, const std::string& token,
                                  const std::string& context = std::string())
    {
        if (isNumericString(token)) return (unsigned int)strtoull(token.c_str(), NULL, 0);
        os.addWarning("Cannot map enum token '" + token + "'" +
                      (context.empty() ? std::string() : (" (" + context + ")")) +
                      " to a numeric value for binary output (writes 0)");
        return 0;
    }

    std::string hexString(unsigned long long value)
    {
        std::ostringstream ss; ss << "0x" << std::hex << value; return ss.str();
    }

    bool writeListOfObjects(OutputStream& os, OutputUserData& ud, const std::string& prop)
    {
        std::vector<int> targets;
        const IRCommand* cmd = ud.findCommand(prop);
        if (cmd)
        {
            for (size_t i = 0; i < cmd->args.size(); ++i)
                if (cmd->args[i].kind == IRArg::OBJECT)
                    targets.push_back(ud.resolve(cmd->args[i]));
        }
        else
        {
            collectIndexedObjects(ud, "set" + prop, targets);
            if (targets.empty() && prop.size() > 4 &&
                prop.compare(prop.size() - 4, 4, "List") == 0)
            {
                // Legacy indexed form of a modern object list:
                // "TexCoordArrayList" <- "setTexCoordArray i obj".
                collectIndexedObjects(ud, "set" + prop.substr(0, prop.size() - 4), targets);
            }
        }

        if (!os.isBinary()) os << os.PROPERTY(prop);
        os << (unsigned int)targets.size();
        if (!targets.empty())
        {
            if (!os.isBinary()) os << os.BEGIN_BRACKET << std::endl;
            for (size_t i = 0; i < targets.size(); ++i)
            {
                os.writeObject(ud, targets[i]);
                if (!os.isBinary()) os << std::endl;
            }
            if (!os.isBinary()) os << os.END_BRACKET << std::endl;
        }
        else if (!os.isBinary()) os << std::endl;
        return true;
    }

    template<typename T>
    bool writeListOfValues(OutputStream& os, OutputUserData& ud, const std::string& prop)
    {
        std::vector<T> values;
        const IRCommand* cmd = ud.findCommand(prop);
        if (cmd)
        {
            for (size_t i = 0; i < cmd->args.size(); ++i)
            {
                const IRArg* a = &cmd->args[i];
                if (a->kind == IRArg::OBJECT) continue;
                values.push_back((T)irToDouble(a, 0.0));
            }
        }
        else
        {
            // The reader does not export primitive ListSerializer values yet
            // (e.g. osg::Switch::ValueList); make the resulting empty list
            // visible instead of silently dropping it.
            os.addWarning("No captured values for list property '" + prop + "'");
        }

        if (!os.isBinary()) os << os.PROPERTY(prop);
        os << (unsigned int)values.size();
        if (!values.empty())
        {
            if (!os.isBinary()) os << os.BEGIN_BRACKET << std::endl;
            for (size_t i = 0; i < values.size(); ++i)
            {
                os << values[i];
                if (!os.isBinary()) os << std::endl;
            }
            if (!os.isBinary()) os << os.END_BRACKET << std::endl;
        }
        else if (!os.isBinary()) os << std::endl;
        return true;
    }

    template<typename T>
    bool tryWriteList(OutputStream& os, OutputUserData& ud,
                      const std::string& prop, BaseSerializer& s)
    {
        if (typeid(s) != typeid(ListSerializer<T>)) return false;
        ListSerializer<T>& ls = static_cast<ListSerializer<T>&>(s);
        (void)ls;
        return writeListOfValues<T>(os, ud, prop);
    }

    template<typename T>
    bool tryWriteValue(OutputStream& os, OutputUserData& ud,
                       const std::string& prop, BaseSerializer& s)
    {
        if (typeid(s) != typeid(ValueSerializer<T>)) return false;
        ValueSerializer<T>& vs = static_cast<ValueSerializer<T>&>(s);

        T value = vs._default;
        const IRArg* a = ud.findPropertyArg(prop);
        if (a) assignFromArg(*a, value);
        else if constexpr (std::is_arithmetic<T>::value)
        {
            // PrimitiveSet payload fallbacks: those values live in the IR object,
            // not in a command (the compact reader consumes them positionally).
            const IRObject* obj = ud.currentObject();
            if (obj)
            {
                if (prop == "First") value = (T)obj->primFirst;
                else if (prop == "Count") value = (T)obj->primCount;
            }
        }

        if (!os.isBinary()) os << os.PROPERTY(prop);
        if (vs._hex && !os.isBinary())
        {
            unsigned long long hv = 0;
            if (a) hv = irToUInt(a, 0);
            else if constexpr (std::is_arithmetic<T>::value)
                hv = (unsigned long long)(long long)value;
            os << hexString(hv);
        }
        else
            os << value;
        if (!os.isBinary()) os << std::endl;
        return true;
    }
}

namespace osgVerse
{
    bool writeSerializerValue(OutputStream& os, OutputUserData& ud,
                              const std::string& prop, BaseSerializer& s)
    {
        // ---- Bool ----
        if (BoolSerializer* bs = dynamic_cast<BoolSerializer*>(&s))
        {
            bool v = irToBool(ud.findPropertyArg(prop), bs->_default);
            if (!os.isBinary()) os << os.PROPERTY(prop);
            os << v;
            if (!os.isBinary()) os << std::endl;
            return true;
        }

        // ---- String ----
        if (StringSerializer* ss = dynamic_cast<StringSerializer*>(&s))
        {
            const IRArg* a = ud.findPropertyArg(prop);
            std::string v = a ? irToString(a, ss->_default) : ss->_default;
            if (!os.isBinary()) os << os.PROPERTY(prop);
            os.writeWrappedString(v);
            if (!os.isBinary()) os << std::endl;
            return true;
        }

        // ---- Enum: ASCII keeps the token, binary stores the integer ----
        if (EnumSerializer* es = dynamic_cast<EnumSerializer*>(&s))
        {
            const IRArg* a = ud.findPropertyArg(prop);
            std::string token = a ? irToString(a, es->_default) : es->_default;

            // osg::StateSet::setRenderingHint() also derives the render-bin mode.
            // An ASCII file that only stored RenderingHint has no explicit
            // RenderBinMode property, but the in-memory StateSet does.
            if (!a && prop == "RenderBinMode")
            {
                const IRArg* hint = ud.findPropertyArg("RenderingHint");
                int h = hint ? (int)irArgInt(*hint) : 0;
                if (h == 1 || h == 2) token = "USE_RENDERBIN_DETAILS";  // OPAQUE_BIN / TRANSPARENT_BIN
            }

            if (!os.isBinary())
            {
                // Prefer the token spelling in ASCII.  IR read from a binary file
                // carries the numeric value, so invert the registered mapping.
                std::string outToken = token;
                if (isNumericString(token))
                {
                    int value = (int)strtol(token.c_str(), NULL, 0);
                    for (std::map<std::string, int>::const_iterator
                         itr = es->_valueMap.begin(); itr != es->_valueMap.end(); ++itr)
                    {
                        if (itr->second == value) { outToken = itr->first; break; }
                    }
                }
                os << os.PROPERTY(prop) << outToken << std::endl;
            }
            else
            {
                std::map<std::string, int>::const_iterator itr = es->_valueMap.find(token);
                if (itr != es->_valueMap.end()) os << (unsigned int)itr->second;
                else
                {
                    std::string className;
                    if (const IRObject* obj = ud.currentObject()) className = obj->className;
                    os << enumTokenToValue(os, token, prop + " of " + className);
                }
            }
            return true;
        }

        // ---- GLenum ----
        if (GLenumSerializer* gs = dynamic_cast<GLenumSerializer*>(&s))
        {
            const IRArg* a = ud.findPropertyArg(prop);
            unsigned int v = (unsigned int)irToUInt(a, gs->_default);

            // The legacy compact primitive reader does not record "Mode".
            if (!a)
            {
                const IRObject* obj = ud.currentObject();
                if (obj && prop == "Mode" && obj->primMode != 0) v = (unsigned int)obj->primMode;
            }

            if (!os.isBinary()) os << os.PROPERTY(prop);
            os << ObjectGLenum(v, gs->_type.c_str());
            if (!os.isBinary()) os << std::endl;
            return true;
        }

        // ---- Bit flags ----
        if (BitFlagSerializer* bfs = dynamic_cast<BitFlagSerializer*>(&s))
        {
            const IRArg* a = ud.findPropertyArg(prop);
            unsigned int v = (unsigned int)irToUInt(a, bfs->_default);
            if (!os.isBinary()) os << os.PROPERTY(prop);
            os << v;
            if (!os.isBinary()) os << std::endl;
            return true;
        }

        // ---- User serializer ----
        if (UserSerializer* us = dynamic_cast<UserSerializer*>(&s))
        {
            if (us->_writer)
            {
                bool has = us->_checker ? us->_checker(ud) : true;
                if (os.isBinary())
                {
                    os << has;
                    if (has) us->_writer(os, ud);
                }
                else if (has)
                {
                    os << os.PROPERTY(prop);
                    us->_writer(os, ud);
                }
            }
            else
            {
                // Binary readers need the "has value" flag; ASCII properties
                // may simply be omitted.
                if (os.isBinary()) os << false;
                ud.addWarning("No writer for property '" + prop + "'");
            }
            return true;
        }

        // ---- Object / Image ----
        if (ObjectSerializer* os_ = dynamic_cast<ObjectSerializer*>(&s))
        {
            const IRArg* a = ud.findPropertyArg(prop);
            int target = a ? ud.resolve(*a) : -1;
            if (os.isBinary())
            {
                os << (target >= 0);
                if (target >= 0) os.writeObject(ud, target);
            }
            else if (target >= 0)
            {
                os << os.PROPERTY(prop) << true << os.BEGIN_BRACKET << std::endl;
                os.writeObject(ud, target);
                os << os.END_BRACKET << std::endl;
            }
            return true;
        }

        if (ImageSerializer* is_ = dynamic_cast<ImageSerializer*>(&s))
        {
            const IRArg* a = ud.findPropertyArg(prop);
            int target = a ? ud.resolve(*a) : -1;
            if (os.isBinary())
            {
                os << (target >= 0);
                if (target >= 0) os.writeImage(ud, target);
            }
            else if (target >= 0)
            {
                os << os.PROPERTY(prop) << true << os.BEGIN_BRACKET << std::endl;
                os.writeImage(ud, target);
                os << os.END_BRACKET << std::endl;
            }
            return true;
        }

        // ---- Object lists (PrimitiveSetList / TexCoordArrayList / ...) ----
        if (dynamic_cast<ListSerializer<ObjectTypeAndID>*>(&s))
            return writeListOfObjects(os, ud, prop);

        // ---- VectorSerializer (array / DrawElements payload) ----
        if (VectorSerializer* vs = dynamic_cast<VectorSerializer*>(&s))
        {
            const IRObject* obj = ud.currentObject();
            std::vector<double> data;
            int components = 0;
            std::string token = vs->_type;

            if (obj)
            {
                data = obj->vectorData;
                components = obj->vectorComponents;
                if (!obj->vectorToken.empty()) token = obj->vectorToken;
                if (data.empty() && !obj->primIndices.empty())
                {
                    data.reserve(obj->primIndices.size());
                    for (size_t i = 0; i < obj->primIndices.size(); ++i)
                        data.push_back((double)obj->primIndices[i]);
                    components = 1;
                }
                else if (data.empty() && !obj->primLengths.empty())
                {
                    data.reserve(obj->primLengths.size());
                    for (size_t i = 0; i < obj->primLengths.size(); ++i)
                        data.push_back((double)obj->primLengths[i]);
                    components = 1;
                }
            }

            if (!os.isBinary()) os << os.PROPERTY(prop);
            return writeVectorValues(os, token, data, components,
                                     obj ? &obj->vectorStrings : NULL);
        }

        // ---- Primitive value lists (TimeList / ValueList ...) ----
        if (tryWriteList<double>(os, ud, prop, s)) return true;
        if (tryWriteList<float>(os, ud, prop, s)) return true;
        if (tryWriteList<bool>(os, ud, prop, s)) return true;
        if (tryWriteList<int>(os, ud, prop, s)) return true;
        if (tryWriteList<unsigned int>(os, ud, prop, s)) return true;

        // ---- ValueSerializer<T> scalars ----
        if (tryWriteValue<char>(os, ud, prop, s)) return true;
        if (tryWriteValue<signed char>(os, ud, prop, s)) return true;
        if (tryWriteValue<unsigned char>(os, ud, prop, s)) return true;
        if (tryWriteValue<short>(os, ud, prop, s)) return true;
        if (tryWriteValue<unsigned short>(os, ud, prop, s)) return true;
        if (tryWriteValue<int>(os, ud, prop, s)) return true;
        if (tryWriteValue<unsigned int>(os, ud, prop, s)) return true;
        if (tryWriteValue<long>(os, ud, prop, s)) return true;
        if (tryWriteValue<unsigned long>(os, ud, prop, s)) return true;
        if (tryWriteValue<float>(os, ud, prop, s)) return true;
        if (tryWriteValue<double>(os, ud, prop, s)) return true;

        // ---- ValueSerializer<T> vectors / math types ----
        if (tryWriteValue<osg::Vec2b>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec2ub>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec2s>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec2us>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec2i>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec2ui>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec2f>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec2d>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3b>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3ub>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3s>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3us>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3i>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3ui>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3f>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec3d>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4b>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4ub>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4s>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4us>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4i>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4ui>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4f>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Vec4d>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Quat>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Plane>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Matrixf>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::Matrixd>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::BoundingBoxf>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::BoundingBoxd>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::BoundingSpheref>(os, ud, prop, s)) return true;
        if (tryWriteValue<osg::BoundingSphered>(os, ud, prop, s)) return true;

        ud.addWarning("Unhandled serializer for property '" + prop + "'");
        return false;
    }
}
