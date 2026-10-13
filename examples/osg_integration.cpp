// osg_integration: the OpenSceneGraph "echo back" adapter.
//
// The zero-dependency generic_wrappers reader parses the .osgb/.osgt file into a
// neutral SceneIR (records object creation, every serializer property and every
// explicit command). This translation unit only depends on the osg SDK and on
// the osg-free SceneIR.h, so the mini-osg math layer never clashes with real osg.
//
// OsgSceneBuilder implements ISceneBuilder: it instantiates osg objects through
// OSG's own ObjectWrapperManager (no hand-written factory table) and replays the
// recorded properties/commands, rebuilding a renderable scene graph.
//
// Build: -DENABLE_OSG_INTEGRATION=ON -DOSG_INTEGRATION_ROOT=<sdk>
// Usage: osg_integration <file.osgt|file.osgb> [--list]

#include <osg/Object>
#include <osg/Node>
#include <osg/Group>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/StateAttribute>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/Drawable>
#include <osg/Program>
#include <osg/Shader>
#include <osg/Uniform>
#include <osg/Point>
#include <osgDB/ObjectWrapper>
#include <osgDB/Registry>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osgViewer/Viewer>
#include <osg/io_utils>

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <cstring>

#include "SceneIR.h"

namespace
{
    std::string nsFor(const std::string& className)
    {
        if (className == "osg::Image") return "I";
        if (className.find("Array") != std::string::npos) return "A";
        if (className.find("PrimitiveSet") != std::string::npos ||
            className.find("DrawArrays") != std::string::npos ||
            className.find("DrawElements") != std::string::npos) return "P";
        return "O";
    }
}

class OsgSceneBuilder : public osgVerse::ISceneBuilder
{
public:
    void onObject(const osgVerse::IRObject& obj, int index) override
    {
        osg::ref_ptr<osg::Object> o = createInstance(obj.className);
        while ((int)_objs.size() < index) _objs.push_back(osg::ref_ptr<osg::Object>());
        _objs.push_back(o);
        if (!o) { ++_unknownClasses[obj.className]; return; }
        ++_created;

        if (!obj.id.empty()) _byKey[nsFor(obj.className) + "#" + obj.id] = o.get();
        if (index == 0) { _rootRef = o; _root = o.get(); }

        // Array element data (legacy compact arrays fill obj.vectorData;
        // modern arrays are filled via the "vector" serializer command path).
        if (!obj.vectorData.empty())
            fillVectorData(o.get(), obj.vectorToken, obj.vectorData, obj.vectorComponents);

        // Primitive sets from the legacy compact reader.
        if (!obj.primIndices.empty() || obj.primMode != 0 || obj.primCount != 0)
        {
            if (osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(o.get()))
            {
                da->setMode((osg::PrimitiveSet::Mode)obj.primMode);
                da->setFirst(obj.primFirst); da->setCount(obj.primCount);
            }
            else if (osg::DrawElements* de = dynamic_cast<osg::DrawElements*>(o.get()))
            {
                de->setMode((osg::PrimitiveSet::Mode)obj.primMode);
                de->resizeElements((unsigned int)obj.primIndices.size());
                if (osg::DrawElementsUShort* d = dynamic_cast<osg::DrawElementsUShort*>(de))
                { for (size_t i = 0; i < obj.primIndices.size(); ++i) (*d)[i] = (unsigned short)obj.primIndices[i]; }
                else if (osg::DrawElementsUInt* d = dynamic_cast<osg::DrawElementsUInt*>(de))
                { for (size_t i = 0; i < obj.primIndices.size(); ++i) (*d)[i] = obj.primIndices[i]; }
                else if (osg::DrawElementsUByte* d = dynamic_cast<osg::DrawElementsUByte*>(de))
                { for (size_t i = 0; i < obj.primIndices.size(); ++i) (*d)[i] = (unsigned char)obj.primIndices[i]; }
            }
        }

        // Images (external file name or inline pixel data).
        if (osg::Image* img = dynamic_cast<osg::Image*>(o.get()))
            buildImage(img, obj);
    }

    void onCommand(const osgVerse::IRObject& obj, int index,
                   const osgVerse::IRCommand& cmd) override
    {
        osg::Object* target = (index >= 0 && index < (int)_objs.size()) ? _objs[index].get() : NULL;
        if (!target) return;
        dispatch(target, obj, cmd.name, cmd.args);
    }

    osg::Node* root() const { return dynamic_cast<osg::Node*>(_root); }
    unsigned int created() const { return _created; }
    const std::map<std::string, unsigned int>& unknownClasses() const { return _unknownClasses; }
    const std::map<std::string, unsigned int>& unhandled() const { return _unhandled; }

protected:
    static osg::Object* createInstanceImpl(const std::string& className)
    {
        osgDB::ObjectWrapperManager* m = osgDB::Registry::instance()->getObjectWrapperManager();
        if (!m) return NULL;
        osgDB::ObjectWrapper* w = m->findWrapper(className);
        return w ? w->createInstance() : NULL;
    }
    static osg::ref_ptr<osg::Object> createInstance(const std::string& className)
    { return osg::ref_ptr<osg::Object>(createInstanceImpl(className)); }

    osg::Object* resolve(const osgVerse::IRArg& a) const
    {
        if (a.kind != osgVerse::IRArg::OBJECT || a.objId.empty()) return NULL;
        std::map<std::string, osg::Object*>::const_iterator itr =
            _byKey.find(nsFor(a.objType) + "#" + a.objId);
        return itr != _byKey.end() ? itr->second : NULL;
    }

    static const osgVerse::IRArg& arg(const std::vector<osgVerse::IRArg>& a, size_t i)
    { static osgVerse::IRArg empty; return i < a.size() ? a[i] : empty; }
    static GLenum glEnum(const std::vector<osgVerse::IRArg>& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (GLenum)(v.kind == osgVerse::IRArg::INT ? v.i : v.u); }
    static unsigned int uArg(const std::vector<osgVerse::IRArg>& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (unsigned int)(v.kind == osgVerse::IRArg::INT ? v.i : v.u); }
    static int iArg(const std::vector<osgVerse::IRArg>& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (int)(v.kind == osgVerse::IRArg::INT ? v.i : (long long)v.u); }
    static double dArg(const std::vector<osgVerse::IRArg>& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return v.kind == osgVerse::IRArg::DOUBLE ? v.d : (double)v.i; }
    static bool bArg(const std::vector<osgVerse::IRArg>& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i);
      return v.kind == osgVerse::IRArg::BOOL ? v.b : (v.kind == osgVerse::IRArg::UINT ? v.u != 0 : v.i != 0); }

    void dispatch(osg::Object* o, const osgVerse::IRObject& obj,
                  const std::string& name, const std::vector<osgVerse::IRArg>& args)
    {
        // An absent object reference (e.g. an empty vertex-attrib slot, which the
        // writer still emits) means "nothing to set": ignore it silently.
        if (!args.empty() && args[0].kind == osgVerse::IRArg::OBJECT && args[0].objId.empty())
            return;

        // ---- node / drawable composition ----
        if (name == "addChild")
        {
            osg::Group* g = dynamic_cast<osg::Group*>(o);
            osg::Node* n = dynamic_cast<osg::Node*>(resolve(arg(args, 0)));
            if (g && n) g->addChild(n); else ++_unhandled[name];
        }
        else if (name == "addDrawable")
        {
            osg::Geode* g = dynamic_cast<osg::Geode*>(o);
            osg::Drawable* d = dynamic_cast<osg::Drawable*>(resolve(arg(args, 0)));
            if (g && d) g->addDrawable(d); else ++_unhandled[name];
        }
        // ---- state set ----
        else if (name == "setMode")
        {
            osg::StateSet* ss = dynamic_cast<osg::StateSet*>(o);
            if (ss) ss->setMode(glEnum(args, 0), (osg::StateAttribute::GLModeValue)uArg(args, 1));
            else ++_unhandled[name];
        }
        else if (name == "setTextureMode")
        {
            osg::StateSet* ss = dynamic_cast<osg::StateSet*>(o);
            if (ss) ss->setTextureMode(uArg(args, 0), glEnum(args, 1),
                                       (osg::StateAttribute::GLModeValue)uArg(args, 2));
            else ++_unhandled[name];
        }
        else if (name == "setAttribute")
        {
            osg::StateSet* ss = dynamic_cast<osg::StateSet*>(o);
            osg::StateAttribute* a = dynamic_cast<osg::StateAttribute*>(resolve(arg(args, 0)));
            if (ss && a) ss->setAttribute(a, (osg::StateAttribute::OverrideValue)uArg(args, 1));
            else ++_unhandled[name];
        }
        else if (name == "setTextureAttribute")
        {
            osg::StateSet* ss = dynamic_cast<osg::StateSet*>(o);
            osg::StateAttribute* a = dynamic_cast<osg::StateAttribute*>(resolve(arg(args, 1)));
            if (ss && a) ss->setTextureAttribute(uArg(args, 0), a,
                                                 (osg::StateAttribute::OverrideValue)uArg(args, 2));
            else ++_unhandled[name];
        }
        else if (name == "addUniform")
        {
            osg::StateSet* ss = dynamic_cast<osg::StateSet*>(o);
            osg::Uniform* u = dynamic_cast<osg::Uniform*>(resolve(arg(args, 0)));
            if (ss && u) ss->addUniform(u); else ++_unhandled[name];
        }
        // ---- material ----
        else if (name == "setAmbient" || name == "setDiffuse" || name == "setSpecular" ||
                 name == "setEmission")
        {
            osg::Material* m = dynamic_cast<osg::Material*>(o);
            if (m && args.size() >= 2)
            {
                osg::Material::Face face = (osg::Material::Face)iArg(args, 0);
                osg::Vec4f c((float)args[1].v[0], (float)args[1].v[1],
                             (float)args[1].v[2], (float)args[1].v[3]);
                if (name == "setAmbient") m->setAmbient(face, c);
                else if (name == "setDiffuse") m->setDiffuse(face, c);
                else if (name == "setSpecular") m->setSpecular(face, c);
                else m->setEmission(face, c);
            }
            else ++_unhandled[name];
        }
        else if (name == "setShininess")
        {
            osg::Material* m = dynamic_cast<osg::Material*>(o);
            if (m && args.size() >= 2) m->setShininess((osg::Material::Face)iArg(args, 0), (float)dArg(args, 1));
            else ++_unhandled[name];
        }
        // ---- program / shader ----
        else if (name == "setShaderSource")
        {
            osg::Shader* s = dynamic_cast<osg::Shader*>(o);
            if (s && !args.empty()) s->setShaderSource(args[0].s); else ++_unhandled[name];
        }
        else if (name == "setParameter")
        {
            osg::Program* p = dynamic_cast<osg::Program*>(o);
            if (p && args.size() >= 2) p->setParameter((GLint)iArg(args, 0), (GLint)iArg(args, 1));
            else ++_unhandled[name];
        }
        else if (name == "addBindAttribLocation")
        {
            osg::Program* p = dynamic_cast<osg::Program*>(o);
            if (p && args.size() >= 2) p->addBindAttribLocation(args[0].s, uArg(args, 1));
            else ++_unhandled[name];
        }
        // ---- point / drawable flags ----
        else if (name == "Size" || name == "MaxSize" || name == "FadeThresholdSize")
        {
            osg::Point* pt = dynamic_cast<osg::Point*>(o);
            if (pt)
            {
                if (name == "Size") pt->setSize((float)dArg(args, 0));
                else if (name == "MaxSize") pt->setMaxSize((float)dArg(args, 0));
                else pt->setFadeThresholdSize((float)dArg(args, 0));
            }
            else ++_unhandled[name];
        }
        else if (name == "DistanceAttenuation")
        {
            osg::Point* pt = dynamic_cast<osg::Point*>(o);
            if (pt && !args.empty() && args[0].n >= 3)
                pt->setDistanceAttenuation(osg::Vec3(args[0].v[0], args[0].v[1], args[0].v[2]));
            else ++_unhandled[name];
        }
        else if (name == "UseDisplayList" || name == "SupportsDisplayList")
        {
            osg::Drawable* d = dynamic_cast<osg::Drawable*>(o);
            if (d)
            {
                if (name == "UseDisplayList") d->setUseDisplayList(!args.empty() && bArg(args, 0));
                else d->setSupportsDisplayList(!args.empty() && bArg(args, 0));
            }
            else ++_unhandled[name];
        }
        else if (name == "addShader")
        {
            osg::Program* p = dynamic_cast<osg::Program*>(o);
            osg::Shader* s = dynamic_cast<osg::Shader*>(resolve(arg(args, 0)));
            if (p && s) p->addShader(s); else ++_unhandled[name];
        }
        // ---- LOD ----
        else if (name == "setRange")
        {
            osg::LOD* l = dynamic_cast<osg::LOD*>(o);
            if (l && args.size() >= 3) l->setRange(uArg(args, 0), (float)dArg(args, 1), (float)dArg(args, 2));
            else ++_unhandled[name];
        }
        // ---- object / node properties ----
        else if (name == "Name") { o->setName(args.empty() ? std::string() : args[0].s); }
        else if (name == "DataVariance")
        {
            osg::Object::DataVariance dv = osg::Object::UNSPECIFIED;
            if (!args.empty() && args[0].s == "STATIC") dv = osg::Object::STATIC;
            else if (!args.empty() && args[0].s == "DYNAMIC") dv = osg::Object::DYNAMIC;
            o->setDataVariance(dv);
        }
        else if (name == "UserDataContainer")
        {
            osg::Object* udc = resolve(arg(args, 0));
            if (udc) o->setUserDataContainer(dynamic_cast<osg::UserDataContainer*>(udc));
        }
        else if (name == "Matrix")
        {
            osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(o);
            if (mt && !args.empty() && args[0].kind == osgVerse::IRArg::MATRIX)
            { osg::Matrixd m; for (int i = 0; i < 16; ++i) m(i % 4, i / 4) = args[0].v[i]; mt->setMatrix(m); }
            else ++_unhandled[name];
        }
        else if (name == "Center")
        {
            osg::LOD* l = dynamic_cast<osg::LOD*>(o);
            if (l && !args.empty() && args[0].n >= 3)
                l->setCenter(osg::Vec3d(args[0].v[0], args[0].v[1], args[0].v[2]));
            else ++_unhandled[name];
        }
        else if (name == "Radius")
        {
            osg::LOD* l = dynamic_cast<osg::LOD*>(o);
            if (l && !args.empty()) l->setRadius((float)dArg(args, 0)); else ++_unhandled[name];
        }
        // ---- primitive set ----
        else if (name == "Mode")
        {
            osg::PrimitiveSet* ps = dynamic_cast<osg::PrimitiveSet*>(o);
            if (ps && !args.empty()) ps->setMode((osg::PrimitiveSet::Mode)uArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "NumInstances")
        {
            osg::PrimitiveSet* ps = dynamic_cast<osg::PrimitiveSet*>(o);
            if (ps && !args.empty()) ps->setNumInstances(iArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "First")
        {
            osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(o);
            if (da && !args.empty()) da->setFirst(iArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "Count")
        {
            osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(o);
            if (da && !args.empty()) da->setCount(iArg(args, 0)); else ++_unhandled[name];
        }
        // ---- array ----
        else if (name == "Binding")
        {
            osg::Array* a = dynamic_cast<osg::Array*>(o);
            if (a) a->setBinding(arrayBinding(args.empty() ? std::string() : args[0].s)); else ++_unhandled[name];
        }
        else if (name == "Normalize")
        {
            osg::Array* a = dynamic_cast<osg::Array*>(o);
            if (a) a->setNormalize(!args.empty() && bArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "PreserveDataType")
        {
            osg::Array* a = dynamic_cast<osg::Array*>(o);
            if (a) a->setPreserveDataType(!args.empty() && bArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "vector") { /* already applied in onObject() */ }
        // ---- texture ----
        else if (name == "WrapS" || name == "WrapT" || name == "WrapR")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t)
            {
                osg::Texture::WrapParameter p = (name == "WrapS") ? osg::Texture::WRAP_S :
                    (name == "WrapT" ? osg::Texture::WRAP_T : osg::Texture::WRAP_R);
                t->setWrap(p, (osg::Texture::WrapMode)uArg(args, 0));
            }
            else ++_unhandled[name];
        }
        else if (name == "FilterMin" || name == "FilterMag")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t)
            {
                osg::Texture::FilterParameter p =
                    (name == "FilterMin") ? osg::Texture::MIN_FILTER : osg::Texture::MAG_FILTER;
                t->setFilter(p, (osg::Texture::FilterMode)uArg(args, 0));
            }
            else ++_unhandled[name];
        }
        else if (name == "InternalFormat")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t) t->setInternalFormat(iArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "Image")
        {
            osg::Texture2D* t = dynamic_cast<osg::Texture2D*>(o);
            osg::Image* img = dynamic_cast<osg::Image*>(resolve(arg(args, 0)));
            if (t && img) t->setImage(img);   // images are not recorded yet -> skipped
        }
        // ---- geometry arrays ----
        else if (name == "VertexArray" || name == "NormalArray" || name == "ColorArray" ||
                 name == "SecondaryColorArray" || name == "FogCoordArray")
        {
            osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
            osg::Array* a = dynamic_cast<osg::Array*>(resolve(arg(args, 0)));
            if (g && a)
            {
                if (name == "VertexArray") g->setVertexArray(a);
                else if (name == "NormalArray") g->setNormalArray(a);
                else if (name == "ColorArray") g->setColorArray(a);
                else if (name == "SecondaryColorArray") g->setSecondaryColorArray(a);
                else g->setFogCoordArray(a);
            }
            else ++_unhandled[name];
        }
        else if (name == "PrimitiveSetList")
        {
            osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
            if (g)
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    osg::PrimitiveSet* ps = dynamic_cast<osg::PrimitiveSet*>(resolve(args[i]));
                    if (ps) g->addPrimitiveSet(ps);
                }
            }
            else ++_unhandled[name];
        }
        else if (name == "TexCoordArrayList" || name == "VertexAttribArrayList")
        {
            osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
            if (g)
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    osg::Array* a = dynamic_cast<osg::Array*>(resolve(args[i]));
                    if (!a) continue;
                    if (name == "TexCoordArrayList") g->setTexCoordArray((unsigned int)i, a);
                    else g->setVertexAttribArray((unsigned int)i, a);
                }
            }
            else ++_unhandled[name];
        }
        // ---- legacy (fileVersion < 112) command names ----
        else if (name == "setVertexArray" || name == "setNormalArray" || name == "setColorArray" ||
                 name == "setSecondaryColorArray" || name == "setFogCoordArray")
        {
            osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
            osg::Array* a = dynamic_cast<osg::Array*>(resolve(arg(args, 0)));
            if (g && a)
            {
                if (name == "setVertexArray") g->setVertexArray(a);
                else if (name == "setNormalArray") g->setNormalArray(a);
                else if (name == "setColorArray") g->setColorArray(a);
                else if (name == "setSecondaryColorArray") g->setSecondaryColorArray(a);
                else g->setFogCoordArray(a);
            }
            else ++_unhandled[name];
        }
        else if (name == "setTexCoordArray")
        {
            osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
            osg::Array* a = dynamic_cast<osg::Array*>(resolve(arg(args, 1)));
            if (g && a) g->setTexCoordArray(uArg(args, 0), a); else ++_unhandled[name];
        }
        else if (name == "setVertexAttribArray")
        {
            osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
            osg::Array* a = dynamic_cast<osg::Array*>(resolve(arg(args, 1)));
            if (g && a) g->setVertexAttribArray(uArg(args, 0), a); else ++_unhandled[name];
        }
        else if (name == "setWrap")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t) t->setWrap((osg::Texture::WrapParameter)uArg(args, 0),
                              (osg::Texture::WrapMode)uArg(args, 1));
            else ++_unhandled[name];
        }
        else if (name == "setFilter")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t) t->setFilter((osg::Texture::FilterParameter)uArg(args, 0),
                                (osg::Texture::FilterMode)uArg(args, 1));
            else ++_unhandled[name];
        }
        else if (name == "setInternalFormat")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t) t->setInternalFormat(iArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "setSourceFormat")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t) t->setSourceFormat((GLint)iArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "setSourceType")
        {
            osg::Texture* t = dynamic_cast<osg::Texture*>(o);
            if (t) t->setSourceType((GLenum)uArg(args, 0)); else ++_unhandled[name];
        }
        else if (name == "setSwizzle") { /* not needed for rendering */ }
        // ---- node / drawable state & bounds ----
        else if (name == "StateSet" || name == "setStateSet")
        {
            osg::StateSet* ss = dynamic_cast<osg::StateSet*>(resolve(arg(args, 0)));
            bool ok = false;
            if (ss)
            {
                if (osg::Node* n = dynamic_cast<osg::Node*>(o)) { n->setStateSet(ss); ok = true; }
                else if (osg::Drawable* d = dynamic_cast<osg::Drawable*>(o)) { d->setStateSet(ss); ok = true; }
            }
            if (!ok) ++_unhandled[name];
        }
        else if (name == "setInitialBound")
        {
            bool ok = false;
            if (osg::Node* n = dynamic_cast<osg::Node*>(o))
            {
                if (!args.empty() && args[0].kind == osgVerse::IRArg::BSPHERE)
                {
                    n->setInitialBound(osg::BoundingSphere(
                        osg::Vec3d(args[0].v[0], args[0].v[1], args[0].v[2]), args[0].v[3]));
                    ok = true;
                }
            }
            else if (osg::Drawable* d = dynamic_cast<osg::Drawable*>(o))
            {
                if (!args.empty() && args[0].kind == osgVerse::IRArg::BBOX)
                {
                    d->setInitialBound(osg::BoundingBox(
                        osg::Vec3(args[0].v[0], args[0].v[1], args[0].v[2]),
                        osg::Vec3(args[0].v[3], args[0].v[4], args[0].v[5])));
                    ok = true;
                }
            }
            if (!ok) ++_unhandled[name];
        }
        else
            ++_unhandled[name];
    }

    static osg::Array::Binding arrayBinding(const std::string& s)
    {
        if (s == "BIND_OFF") return osg::Array::BIND_OFF;
        if (s == "BIND_OVERALL") return osg::Array::BIND_OVERALL;
        if (s == "BIND_PER_PRIMITIVE_SET") return osg::Array::BIND_PER_PRIMITIVE_SET;
        if (s == "BIND_PER_VERTEX") return osg::Array::BIND_PER_VERTEX;
        return osg::Array::BIND_UNDEFINED;
    }

    static void fillVectorData(osg::Object* o, const std::string& token,
                               const std::vector<double>& v, int n)
    {
        if (n <= 0) return;
        size_t count = v.size() / (size_t)n;
        if (osg::Vec3Array* a = dynamic_cast<osg::Vec3Array*>(o))
        { a->resize(count); for (size_t i = 0; i < count; ++i) (*a)[i].set((float)v[i*n], (float)v[i*n+1], (float)v[i*n+2]); }
        else if (osg::Vec2Array* a = dynamic_cast<osg::Vec2Array*>(o))
        { a->resize(count); for (size_t i = 0; i < count; ++i) (*a)[i].set((float)v[i*n], (float)v[i*n+1]); }
        else if (osg::Vec4Array* a = dynamic_cast<osg::Vec4Array*>(o))
        { a->resize(count); for (size_t i = 0; i < count; ++i) (*a)[i].set((float)v[i*n], (float)v[i*n+1], (float)v[i*n+2], (float)v[i*n+3]); }
        else if (osg::FloatArray* a = dynamic_cast<osg::FloatArray*>(o))
        { a->resize(count); for (size_t i = 0; i < count; ++i) (*a)[i] = (float)v[i]; }
        else if (osg::DrawElements* de = dynamic_cast<osg::DrawElements*>(o))
        {
            de->resizeElements((unsigned int)v.size());
            if (osg::DrawElementsUShort* d = dynamic_cast<osg::DrawElementsUShort*>(de))
            { for (size_t i = 0; i < v.size(); ++i) (*d)[i] = (unsigned short)v[i]; }
            else if (osg::DrawElementsUInt* d = dynamic_cast<osg::DrawElementsUInt*>(de))
            { for (size_t i = 0; i < v.size(); ++i) (*d)[i] = (unsigned int)v[i]; }
            else if (osg::DrawElementsUByte* d = dynamic_cast<osg::DrawElementsUByte*>(de))
            { for (size_t i = 0; i < v.size(); ++i) (*d)[i] = (unsigned char)v[i]; }
        }
        else if (osg::DoubleArray* a = dynamic_cast<osg::DoubleArray*>(o))
        { a->resize(count); for (size_t i = 0; i < count; ++i) (*a)[i] = v[i]; }
    }

    static void buildImage(osg::Image* img, const osgVerse::IRObject& obj)
    {
        if (!obj.fileName.empty())
        {
            osg::ref_ptr<osg::Image> loaded = osgDB::readImageFile(obj.fileName);
            if (loaded.valid() && loaded->data())
            {
                unsigned int size = loaded->getTotalSizeInBytes();
                unsigned char* buf = new unsigned char[size];
                std::memcpy(buf, loaded->data(), size);
                img->setImage(loaded->s(), loaded->t(), loaded->r(),
                              loaded->getInternalTextureFormat(), loaded->getPixelFormat(),
                              loaded->getDataType(), buf, osg::Image::USE_NEW_DELETE,
                              loaded->getPacking());
                return;
            }
        }
        if (!obj.pixelData.empty() && obj.width > 0 && obj.height > 0)
        {
            unsigned int size = (unsigned int)obj.pixelData.size();
            unsigned char* buf = new unsigned char[size];
            std::memcpy(buf, &obj.pixelData[0], size);
            img->setImage(obj.width, obj.height, obj.depth > 0 ? obj.depth : 1,
                          obj.internalFormat, (GLenum)obj.pixelFormat, (GLenum)obj.dataType,
                          buf, osg::Image::USE_NEW_DELETE,
                          obj.packing > 0 ? obj.packing : 1);
        }
    }

    std::vector<osg::ref_ptr<osg::Object> > _objs;
    std::map<std::string, osg::Object*> _byKey;
    osg::ref_ptr<osg::Object> _rootRef;
    osg::Object* _root = NULL;
    unsigned int _created = 0;
    std::map<std::string, unsigned int> _unknownClasses;
    std::map<std::string, unsigned int> _unhandled;
};

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: osg_integration <file.osgt|file.osgb> [--list]" << std::endl;
        return 1;
    }
    const std::string filename = argv[1];
    bool listOnly = (argc > 2 && std::string(argv[2]) == "--list");

    // Let relative texture/image file names in the asset resolve.
    {
        std::string dir = osgDB::getFilePath(filename);
        osgDB::Registry::instance()->getDataFilePathList().push_back(dir.empty() ? "." : dir);
    }

    osgVerse::SceneIR ir;
    if (!osgVerse::readSceneIR(filename, ir))
    {
        std::cerr << "Failed to read " << filename << std::endl;
        return 1;
    }

    OsgSceneBuilder builder;
    osgVerse::buildScene(ir, builder);

    std::cout << "IR objects  : " << ir.objects.size() << " (" << ir.events.size() << " events)" << std::endl;
    std::cout << "Rebuilt     : " << builder.created() << " osg objects" << std::endl;

    const std::map<std::string, unsigned int>& unk = builder.unknownClasses();
    if (!unk.empty())
    {
        std::cout << "Unknown osg classes (" << unk.size() << "):" << std::endl;
        for (std::map<std::string, unsigned int>::const_iterator it = unk.begin(); it != unk.end(); ++it)
            std::cout << "  " << it->first << " x" << it->second << std::endl;
    }
    const std::map<std::string, unsigned int>& unh = builder.unhandled();
    if (!unh.empty())
    {
        std::cout << "Unhandled properties/commands (" << unh.size() << "):" << std::endl;
        for (std::map<std::string, unsigned int>::const_iterator it = unh.begin(); it != unh.end(); ++it)
            std::cout << "  " << it->first << " x" << it->second << std::endl;
    }

    osg::Node* root = builder.root();
    if (!root) { std::cerr << "No renderable root rebuilt." << std::endl; return 2; }

    std::cout << "Root        : " << root->className() << std::endl;
    if (listOnly) return 0;

    osgViewer::Viewer viewer;
    viewer.setSceneData(root);
    return viewer.run();
}
