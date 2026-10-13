#include "GenericReserializer.h"
using namespace osgVerse;

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
    else std::cerr << "[SerializerVisitor] No callback for " << typeID.name() << "\n";
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
    // expansion); strip it here.
    void readVectorElement(osgVerse::InputStream& is, const std::string& rawType)
    {
        std::string t = rawType;
        std::string::size_type p = t.rfind("::");
        if (p != std::string::npos) t = t.substr(p + 2);

        if (t == "RW_BOOL") { bool v; is >> v; }
        else if (t == "RW_CHAR") { char v; is >> v; }
        else if (t == "RW_UCHAR") { unsigned char v; is >> v; }
        else if (t == "RW_SHORT") { short v; is >> v; }
        else if (t == "RW_USHORT") { unsigned short v; is >> v; }
        else if (t == "RW_INT") { int v; is >> v; }
        else if (t == "RW_UINT") { unsigned int v; is >> v; }
        else if (t == "RW_LONG") { long v; is >> v; }
        else if (t == "RW_ULONG") { unsigned long v; is >> v; }
        else if (t == "RW_FLOAT") { float v; is >> v; }
        else if (t == "RW_DOUBLE") { double v; is >> v; }
        else if (t == "RW_VEC2B") { osg::Vec2b v; is >> v; }
        else if (t == "RW_VEC2UB") { osg::Vec2ub v; is >> v; }
        else if (t == "RW_VEC2S") { osg::Vec2s v; is >> v; }
        else if (t == "RW_VEC2US") { osg::Vec2us v; is >> v; }
        else if (t == "RW_VEC2I") { osg::Vec2i v; is >> v; }
        else if (t == "RW_VEC2UI") { osg::Vec2ui v; is >> v; }
        else if (t == "RW_VEC2F") { osg::Vec2f v; is >> v; }
        else if (t == "RW_VEC2D") { osg::Vec2d v; is >> v; }
        else if (t == "RW_VEC3B") { osg::Vec3b v; is >> v; }
        else if (t == "RW_VEC3UB") { osg::Vec3ub v; is >> v; }
        else if (t == "RW_VEC3S") { osg::Vec3s v; is >> v; }
        else if (t == "RW_VEC3US") { osg::Vec3us v; is >> v; }
        else if (t == "RW_VEC3I") { osg::Vec3i v; is >> v; }
        else if (t == "RW_VEC3UI") { osg::Vec3ui v; is >> v; }
        else if (t == "RW_VEC3F") { osg::Vec3f v; is >> v; }
        else if (t == "RW_VEC3D") { osg::Vec3d v; is >> v; }
        else if (t == "RW_VEC4B") { osg::Vec4b v; is >> v; }
        else if (t == "RW_VEC4UB") { osg::Vec4ub v; is >> v; }
        else if (t == "RW_VEC4S") { osg::Vec4s v; is >> v; }
        else if (t == "RW_VEC4US") { osg::Vec4us v; is >> v; }
        else if (t == "RW_VEC4I") { osg::Vec4i v; is >> v; }
        else if (t == "RW_VEC4UI") { osg::Vec4ui v; is >> v; }
        else if (t == "RW_VEC4F") { osg::Vec4f v; is >> v; }
        else if (t == "RW_VEC4D") { osg::Vec4d v; is >> v; }
        else if (t == "RW_QUAT") { osg::Quat v; is >> v; }
        else if (t == "RW_PLANE") { osg::Plane v; is >> v; }
        else if (t == "RW_MATRIX" || t == "RW_MATRIXF") { osg::Matrixf v; is >> v; }
        else if (t == "RW_MATRIXD") { osg::Matrixd v; is >> v; }
        else if (t == "RW_BOUNDINGBOXF") { osg::BoundingBoxf v; is >> v; }
        else if (t == "RW_BOUNDINGBOXD" || t == "RW_BOUNDINGBOX") { osg::BoundingBoxd v; is >> v; }
        else if (t == "RW_BOUNDINGSPHEREF") { osg::BoundingSpheref v; is >> v; }
        else if (t == "RW_BOUNDINGSPHERED" || t == "RW_BOUNDINGSPHERE") { osg::BoundingSphered v; is >> v; }
        else if (t == "RW_STRING") { std::string v; is.readWrappedString(v); }
        else if (t == "RW_GLENUM") { osgVerse::ObjectGLenum v; is >> v; }
        else
        {
            std::cerr << "[readVectorElement] Unhandled vector element type: "
                      << rawType << std::endl;
        }
    }

    bool readVectorValue(osgVerse::InputStream& is, osgVerse::BaseSerializer& s)
    {
        osgVerse::VectorSerializer* vs = dynamic_cast<osgVerse::VectorSerializer*>(&s);
        if (!vs) return false;

        unsigned int size = 0;
        is >> size;
        if (size > 0)
        {
            // Standard osg binary: VectorSerializer has no brackets (size + elements)
            if (!is.isBinary()) is >> is.BEGIN_BRACKET;
            for (unsigned int i = 0; i < size; ++i)
            {
                readVectorElement(is, vs->_type);
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
        unsigned int size = 0; is >> size;
        if (size > 0)
        {
            // Standard osg binary: ListSerializer has no brackets (size + element objects);
            // only ASCII mode writes BEGIN_BRACKET/END_BRACKET
            if (!is.isBinary()) is >> is.BEGIN_BRACKET;
            for (unsigned int i = 0; i < size; ++i)
            {
                osgVerse::ObjectTypeAndID oid;
                if (ls._elemType == "osg::PrimitiveSet")
                {
                    // Two element layouts exist (mirrors readObjectFromStream's dispatch):
                    // - osgVerse custom exports (binary, fileVersion >= 112): full-object
                    //   form [className][UniqueID][fields...], e.g. v202 sponza.osgb
                    //   writes "osg::DrawElementsUShort" + EBO ref + index vector.
                    // - Legacy standard format (binary < 112, e.g. v91): compact
                    //   readPrimitiveSet form (type enum + mode + numIndices + data),
                    //   e.g. 53(ID_DRAWELEMENTS_USHORT), 4(GL_TRIANGLES), size, indices.
                    if (is.isBinary() && is.getFileVersion() >= 112)
                        oid = is.readObject(ud);
                    else
                        oid = is.readPrimitiveSet(ud);
                }
                else if (ls._elemType == "osg::Array")
                {
                    // Array list elements: same version split as PrimitiveSet -
                    // custom exports (>= 112) write full objects, legacy/ascii use
                    // the compact readArray form (ArrayID + ArrayType + data).
                    if (is.isBinary() && is.getFileVersion() >= 112)
                        oid = is.readObject(ud);
                    else
                        oid = is.readArray(ud);
                }
                else oid = is.readObject(ud);
                if (i == 0) ls._v0 = oid;
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
            if (is.isBinary())
            {
                bool hasObject = false;
                is >> hasObject;
                if (hasObject) (void)is.readObject(ud);
            }
            else
            {
                (void)ud.readObjectFromStream(is, os_->_type);
            }
            return true;
        }

        // Image serializer
        if (ImageSerializer* is_ = dynamic_cast<ImageSerializer*>(&s))
        {
            if (is.isBinary())
            {
                bool hasObject = false; is >> hasObject;
                if (hasObject) (void)is.readImage(ud);
            }
            else
            {
                bool hasObject = false; is >> hasObject;
                if (hasObject)
                {
                    is >> is.BEGIN_BRACKET;
                    (void)is.readImage(ud);
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
