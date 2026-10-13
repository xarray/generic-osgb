// osg_integration: the OpenSceneGraph "echo back" adapter.
//
// The zero-dependency generic_wrappers reader parses the .osgb/.osgt file into a
// neutral SceneIR (records object creation, every serializer property and every
// explicit command). This translation unit only depends on the osg SDK and on
// the osg-free SceneIR.h, so the mini-osg math layer never clashes with real osg.
//
// OsgSceneBuilder implements ISceneBuilder: it instantiates osg objects through
// OSG's own ObjectWrapperManager (no hand-written factory table) and replays the
// recorded properties/commands, rebuilding a renderable scene graph. Properties
// and commands are dispatched through a name -> handler table, so adding support
// for a new class/property is just adding a handler function and one table line.
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
#include <initializer_list>
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
    OsgSceneBuilder() { registerHandlers(); }

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
            buildPrimitive(o.get(), obj);

        // Images (external file name or inline pixel data).
        if (osg::Image* img = dynamic_cast<osg::Image*>(o.get()))
            buildImage(img, obj);
    }

    void onCommand(const osgVerse::IRObject& obj, int index,
                   const osgVerse::IRCommand& cmd) override
    {
        osg::Object* target = (index >= 0 && index < (int)_objs.size()) ? _objs[index].get() : NULL;
        if (!target) return;
        dispatch(target, cmd.name, cmd.args);
    }

    osg::Node* root() const { return dynamic_cast<osg::Node*>(_root); }
    unsigned int created() const { return _created; }
    const std::map<std::string, unsigned int>& unknownClasses() const { return _unknownClasses; }
    const std::map<std::string, unsigned int>& unhandled() const { return _unhandled; }

protected:
    typedef std::vector<osgVerse::IRArg> Args;

    // A property/command handler: returns true when it applied the value.
    typedef bool (OsgSceneBuilder::*Handler)(osg::Object*, const std::string&, const Args&);

    void on(Handler h, std::initializer_list<const char*> names)
    { for (const char* n : names) _handlers[n] = h; }

    // -----------------------------------------------------------------------
    // Dispatch table: one line per property/command name.
    // -----------------------------------------------------------------------
    void registerHandlers()
    {
        on(&OsgSceneBuilder::handleAddChild, { "addChild" });
        on(&OsgSceneBuilder::handleAddDrawable, { "addDrawable" });
        on(&OsgSceneBuilder::handleStateSet, { "setMode", "setTextureMode", "setAttribute",
                                               "setTextureAttribute", "addUniform" });
        on(&OsgSceneBuilder::handleMaterial, { "setAmbient", "setDiffuse", "setSpecular",
                                               "setEmission", "setShininess" });
        on(&OsgSceneBuilder::handleProgram, { "setShaderSource", "setParameter",
                                              "addBindAttribLocation", "addShader" });
        on(&OsgSceneBuilder::handleLOD, { "setRange", "Center", "Radius" });
        on(&OsgSceneBuilder::handlePoint, { "Size", "MaxSize", "FadeThresholdSize",
                                            "DistanceAttenuation" });
        on(&OsgSceneBuilder::handleDrawableFlags, { "UseDisplayList", "SupportsDisplayList" });
        on(&OsgSceneBuilder::handleObjectProp, { "Name", "DataVariance", "UserDataContainer" });
        on(&OsgSceneBuilder::handleMatrix, { "Matrix" });
        on(&OsgSceneBuilder::handlePrimitive, { "Mode", "NumInstances", "First", "Count" });
        on(&OsgSceneBuilder::handleArrayProp, { "Binding", "Normalize", "PreserveDataType" });
        on(&OsgSceneBuilder::handleVector, { "vector" });
        on(&OsgSceneBuilder::handleTexture, { "WrapS", "WrapT", "WrapR", "FilterMin", "FilterMag",
                                              "InternalFormat", "Image", "setWrap", "setFilter",
                                              "setInternalFormat", "setSourceFormat",
                                              "setSourceType", "setSwizzle" });
        on(&OsgSceneBuilder::handleGeometryArray, { "VertexArray", "NormalArray", "ColorArray",
                                                    "SecondaryColorArray", "FogCoordArray",
                                                    "setVertexArray", "setNormalArray",
                                                    "setColorArray", "setSecondaryColorArray",
                                                    "setFogCoordArray", "setTexCoordArray",
                                                    "setVertexAttribArray" });
        on(&OsgSceneBuilder::handleGeometryList, { "PrimitiveSetList", "TexCoordArrayList",
                                                   "VertexAttribArrayList" });
        on(&OsgSceneBuilder::handleNodeState, { "StateSet", "setStateSet", "setInitialBound" });
    }

    void dispatch(osg::Object* o, const std::string& name, const Args& args)
    {
        // An absent object reference (e.g. an empty vertex-attrib slot, which the
        // writer still emits) means "nothing to set": ignore it silently.
        if (!args.empty() && args[0].kind == osgVerse::IRArg::OBJECT && args[0].objId.empty())
            return;

        std::map<std::string, Handler>::const_iterator itr = _handlers.find(name);
        if (itr == _handlers.end() || !(this->*(itr->second))(o, name, args))
            ++_unhandled[name];
    }

    // -----------------------------------------------------------------------
    // Handlers
    // -----------------------------------------------------------------------
    bool handleAddChild(osg::Object* o, const std::string&, const Args& args)
    {
        osg::Group* g = dynamic_cast<osg::Group*>(o);
        osg::Node* n = dynamic_cast<osg::Node*>(resolve(arg(args, 0)));
        if (!g || !n) return false;
        g->addChild(n); return true;
    }

    bool handleAddDrawable(osg::Object* o, const std::string&, const Args& args)
    {
        osg::Geode* g = dynamic_cast<osg::Geode*>(o);
        osg::Drawable* d = dynamic_cast<osg::Drawable*>(resolve(arg(args, 0)));
        if (!g || !d) return false;
        g->addDrawable(d); return true;
    }

    bool handleStateSet(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::StateSet* ss = dynamic_cast<osg::StateSet*>(o);
        if (!ss) return false;
        if (name == "setMode")
            ss->setMode(glEnum(args, 0), (osg::StateAttribute::GLModeValue)uArg(args, 1));
        else if (name == "setTextureMode")
            ss->setTextureMode(uArg(args, 0), glEnum(args, 1),
                               (osg::StateAttribute::GLModeValue)uArg(args, 2));
        else if (name == "setAttribute")
        {
            osg::StateAttribute* a = dynamic_cast<osg::StateAttribute*>(resolve(arg(args, 0)));
            if (!a) return false;
            ss->setAttribute(a, (osg::StateAttribute::OverrideValue)uArg(args, 1));
        }
        else if (name == "setTextureAttribute")
        {
            osg::StateAttribute* a = dynamic_cast<osg::StateAttribute*>(resolve(arg(args, 1)));
            if (!a) return false;
            ss->setTextureAttribute(uArg(args, 0), a,
                                    (osg::StateAttribute::OverrideValue)uArg(args, 2));
        }
        else // addUniform
        {
            osg::Uniform* u = dynamic_cast<osg::Uniform*>(resolve(arg(args, 0)));
            if (!u) return false;
            ss->addUniform(u);
        }
        return true;
    }

    bool handleMaterial(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::Material* m = dynamic_cast<osg::Material*>(o);
        if (!m || args.size() < 2) return false;
        osg::Material::Face face = (osg::Material::Face)iArg(args, 0);
        if (name == "setShininess") { m->setShininess(face, (float)dArg(args, 1)); return true; }

        osg::Vec4f c((float)args[1].v[0], (float)args[1].v[1],
                     (float)args[1].v[2], (float)args[1].v[3]);
        if (name == "setAmbient") m->setAmbient(face, c);
        else if (name == "setDiffuse") m->setDiffuse(face, c);
        else if (name == "setSpecular") m->setSpecular(face, c);
        else m->setEmission(face, c);
        return true;
    }

    bool handleProgram(osg::Object* o, const std::string& name, const Args& args)
    {
        if (name == "setShaderSource")
        {
            osg::Shader* s = dynamic_cast<osg::Shader*>(o);
            if (!s || args.empty()) return false;
            s->setShaderSource(args[0].s); return true;
        }
        osg::Program* p = dynamic_cast<osg::Program*>(o);
        if (!p) return false;
        if (name == "setParameter")
        {
            if (args.size() < 2) return false;
            p->setParameter((GLint)iArg(args, 0), (GLint)iArg(args, 1)); return true;
        }
        if (name == "addBindAttribLocation")
        {
            if (args.size() < 2) return false;
            p->addBindAttribLocation(args[0].s, uArg(args, 1)); return true;
        }
        osg::Shader* s = dynamic_cast<osg::Shader*>(resolve(arg(args, 0)));   // addShader
        if (!s) return false;
        p->addShader(s); return true;
    }

    bool handleLOD(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::LOD* l = dynamic_cast<osg::LOD*>(o);
        if (!l || args.empty()) return false;
        if (name == "setRange")
        {
            if (args.size() < 3) return false;
            l->setRange(uArg(args, 0), (float)dArg(args, 1), (float)dArg(args, 2)); return true;
        }
        if (name == "Center")
        {
            if (args[0].n < 3) return false;
            l->setCenter(osg::Vec3d(args[0].v[0], args[0].v[1], args[0].v[2])); return true;
        }
        l->setRadius((float)dArg(args, 0)); return true;   // Radius
    }

    bool handlePoint(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::Point* pt = dynamic_cast<osg::Point*>(o);
        if (!pt || args.empty()) return false;
        if (name == "DistanceAttenuation")
        {
            if (args[0].n < 3) return false;
            pt->setDistanceAttenuation(osg::Vec3(args[0].v[0], args[0].v[1], args[0].v[2]));
            return true;
        }
        if (name == "Size") pt->setSize((float)dArg(args, 0));
        else if (name == "MaxSize") pt->setMaxSize((float)dArg(args, 0));
        else pt->setFadeThresholdSize((float)dArg(args, 0));
        return true;
    }

    bool handleDrawableFlags(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::Drawable* d = dynamic_cast<osg::Drawable*>(o);
        if (!d) return false;
        bool v = !args.empty() && bArg(args, 0);
        if (name == "UseDisplayList") d->setUseDisplayList(v);
        else d->setSupportsDisplayList(v);
        return true;
    }

    bool handleObjectProp(osg::Object* o, const std::string& name, const Args& args)
    {
        if (name == "Name") { o->setName(args.empty() ? std::string() : args[0].s); return true; }
        if (name == "DataVariance")
        {
            osg::Object::DataVariance dv = osg::Object::UNSPECIFIED;
            if (!args.empty() && args[0].s == "STATIC") dv = osg::Object::STATIC;
            else if (!args.empty() && args[0].s == "DYNAMIC") dv = osg::Object::DYNAMIC;
            o->setDataVariance(dv); return true;
        }
        osg::Object* udc = resolve(arg(args, 0));   // UserDataContainer
        if (!udc) return false;
        o->setUserDataContainer(dynamic_cast<osg::UserDataContainer*>(udc)); return true;
    }

    bool handleMatrix(osg::Object* o, const std::string&, const Args& args)
    {
        osg::MatrixTransform* mt = dynamic_cast<osg::MatrixTransform*>(o);
        if (!mt || args.empty() || args[0].kind != osgVerse::IRArg::MATRIX) return false;
        osg::Matrixd m; for (int i = 0; i < 16; ++i) m(i % 4, i / 4) = args[0].v[i];
        mt->setMatrix(m); return true;
    }

    bool handlePrimitive(osg::Object* o, const std::string& name, const Args& args)
    {
        if (args.empty()) return false;
        if (name == "Mode" || name == "NumInstances")
        {
            osg::PrimitiveSet* ps = dynamic_cast<osg::PrimitiveSet*>(o);
            if (!ps) return false;
            if (name == "Mode") ps->setMode((osg::PrimitiveSet::Mode)uArg(args, 0));
            else ps->setNumInstances(iArg(args, 0));
            return true;
        }
        osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(o);
        if (!da) return false;
        if (name == "First") da->setFirst(iArg(args, 0)); else da->setCount(iArg(args, 0));
        return true;
    }

    bool handleArrayProp(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::Array* a = dynamic_cast<osg::Array*>(o);
        if (!a) return false;
        if (name == "Binding") a->setBinding(arrayBinding(args.empty() ? std::string() : args[0].s));
        else if (name == "Normalize") a->setNormalize(!args.empty() && bArg(args, 0));
        else a->setPreserveDataType(!args.empty() && bArg(args, 0));
        return true;
    }

    bool handleVector(osg::Object*, const std::string&, const Args&)
    { return true; }   // element data is applied in onObject()

    bool handleTexture(osg::Object* o, const std::string& name, const Args& args)
    {
        if (name == "Image")
        {
            osg::Texture2D* t = dynamic_cast<osg::Texture2D*>(o);
            osg::Image* img = dynamic_cast<osg::Image*>(resolve(arg(args, 0)));
            if (!t || !img) return false;
            t->setImage(img); return true;
        }
        osg::Texture* t = dynamic_cast<osg::Texture*>(o);
        if (!t) return false;
        if (name == "WrapS" || name == "WrapT" || name == "WrapR")
        {
            osg::Texture::WrapParameter p = (name == "WrapS") ? osg::Texture::WRAP_S :
                (name == "WrapT" ? osg::Texture::WRAP_T : osg::Texture::WRAP_R);
            t->setWrap(p, (osg::Texture::WrapMode)uArg(args, 0)); return true;
        }
        if (name == "FilterMin" || name == "FilterMag")
        {
            osg::Texture::FilterParameter p = (name == "FilterMin") ?
                osg::Texture::MIN_FILTER : osg::Texture::MAG_FILTER;
            t->setFilter(p, (osg::Texture::FilterMode)uArg(args, 0)); return true;
        }
        if (name == "InternalFormat" || name == "setInternalFormat")
        { t->setInternalFormat(iArg(args, 0)); return true; }
        if (name == "setWrap")
        {
            t->setWrap((osg::Texture::WrapParameter)uArg(args, 0),
                       (osg::Texture::WrapMode)uArg(args, 1)); return true;
        }
        if (name == "setFilter")
        {
            t->setFilter((osg::Texture::FilterParameter)uArg(args, 0),
                         (osg::Texture::FilterMode)uArg(args, 1)); return true;
        }
        if (name == "setSourceFormat") { t->setSourceFormat((GLint)iArg(args, 0)); return true; }
        if (name == "setSourceType") { t->setSourceType((GLenum)uArg(args, 0)); return true; }
        return true;   // setSwizzle: not needed for rendering
    }

    bool handleGeometryArray(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
        if (!g) return false;
        if (name == "setTexCoordArray" || name == "setVertexAttribArray")
        {
            osg::Array* a = dynamic_cast<osg::Array*>(resolve(arg(args, 1)));
            if (!a) return false;
            if (name == "setTexCoordArray") g->setTexCoordArray(uArg(args, 0), a);
            else g->setVertexAttribArray(uArg(args, 0), a);
            return true;
        }
        osg::Array* a = dynamic_cast<osg::Array*>(resolve(arg(args, 0)));
        if (!a) return false;
        if (name == "VertexArray" || name == "setVertexArray") g->setVertexArray(a);
        else if (name == "NormalArray" || name == "setNormalArray") g->setNormalArray(a);
        else if (name == "ColorArray" || name == "setColorArray") g->setColorArray(a);
        else if (name == "SecondaryColorArray" || name == "setSecondaryColorArray")
            g->setSecondaryColorArray(a);
        else g->setFogCoordArray(a);
        return true;
    }

    bool handleGeometryList(osg::Object* o, const std::string& name, const Args& args)
    {
        osg::Geometry* g = dynamic_cast<osg::Geometry*>(o);
        if (!g) return false;
        if (name == "PrimitiveSetList")
        {
            for (size_t i = 0; i < args.size(); ++i)
                if (osg::PrimitiveSet* ps = dynamic_cast<osg::PrimitiveSet*>(resolve(args[i])))
                    g->addPrimitiveSet(ps);
            return true;
        }
        for (size_t i = 0; i < args.size(); ++i)
        {
            osg::Array* a = dynamic_cast<osg::Array*>(resolve(args[i]));
            if (!a) continue;
            if (name == "TexCoordArrayList") g->setTexCoordArray((unsigned int)i, a);
            else g->setVertexAttribArray((unsigned int)i, a);
        }
        return true;
    }

    bool handleNodeState(osg::Object* o, const std::string&, const Args& args)
    {
        // setInitialBound carries a bounding sphere (Node) or box (Drawable).
        if (!args.empty() && (args[0].kind == osgVerse::IRArg::BSPHERE ||
                              args[0].kind == osgVerse::IRArg::BBOX))
        {
            if (osg::Node* n = dynamic_cast<osg::Node*>(o))
            {
                if (args[0].kind != osgVerse::IRArg::BSPHERE) return false;
                n->setInitialBound(osg::BoundingSphere(
                    osg::Vec3d(args[0].v[0], args[0].v[1], args[0].v[2]), args[0].v[3]));
                return true;
            }
            if (osg::Drawable* d = dynamic_cast<osg::Drawable*>(o))
            {
                if (args[0].kind != osgVerse::IRArg::BBOX) return false;
                d->setInitialBound(osg::BoundingBox(
                    osg::Vec3(args[0].v[0], args[0].v[1], args[0].v[2]),
                    osg::Vec3(args[0].v[3], args[0].v[4], args[0].v[5])));
                return true;
            }
            return false;
        }
        osg::StateSet* ss = dynamic_cast<osg::StateSet*>(resolve(arg(args, 0)));   // StateSet
        if (!ss) return false;
        if (osg::Node* n = dynamic_cast<osg::Node*>(o)) { n->setStateSet(ss); return true; }
        if (osg::Drawable* d = dynamic_cast<osg::Drawable*>(o)) { d->setStateSet(ss); return true; }
        return false;
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
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

    static const osgVerse::IRArg& arg(const Args& a, size_t i)
    { static osgVerse::IRArg empty; return i < a.size() ? a[i] : empty; }
    static GLenum glEnum(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (GLenum)(v.kind == osgVerse::IRArg::INT ? v.i : v.u); }
    static unsigned int uArg(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (unsigned int)(v.kind == osgVerse::IRArg::INT ? v.i : v.u); }
    static int iArg(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (int)(v.kind == osgVerse::IRArg::INT ? v.i : (long long)v.u); }
    static double dArg(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return v.kind == osgVerse::IRArg::DOUBLE ? v.d : (double)v.i; }
    static bool bArg(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i);
      return v.kind == osgVerse::IRArg::BOOL ? v.b : (v.kind == osgVerse::IRArg::UINT ? v.u != 0 : v.i != 0); }

    static osg::Array::Binding arrayBinding(const std::string& s)
    {
        if (s == "BIND_OFF") return osg::Array::BIND_OFF;
        if (s == "BIND_OVERALL") return osg::Array::BIND_OVERALL;
        if (s == "BIND_PER_PRIMITIVE_SET") return osg::Array::BIND_PER_PRIMITIVE_SET;
        if (s == "BIND_PER_VERTEX") return osg::Array::BIND_PER_VERTEX;
        return osg::Array::BIND_UNDEFINED;
    }

    static void buildPrimitive(osg::Object* o, const osgVerse::IRObject& obj)
    {
        if (osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(o))
        {
            da->setMode((osg::PrimitiveSet::Mode)obj.primMode);
            da->setFirst(obj.primFirst); da->setCount(obj.primCount);
            return;
        }
        osg::DrawElements* de = dynamic_cast<osg::DrawElements*>(o);
        if (!de) return;
        de->setMode((osg::PrimitiveSet::Mode)obj.primMode);
        de->resizeElements((unsigned int)obj.primIndices.size());
        if (osg::DrawElementsUShort* d = dynamic_cast<osg::DrawElementsUShort*>(de))
        { for (size_t i = 0; i < obj.primIndices.size(); ++i) (*d)[i] = (unsigned short)obj.primIndices[i]; }
        else if (osg::DrawElementsUInt* d = dynamic_cast<osg::DrawElementsUInt*>(de))
        { for (size_t i = 0; i < obj.primIndices.size(); ++i) (*d)[i] = obj.primIndices[i]; }
        else if (osg::DrawElementsUByte* d = dynamic_cast<osg::DrawElementsUByte*>(de))
        { for (size_t i = 0; i < obj.primIndices.size(); ++i) (*d)[i] = (unsigned char)obj.primIndices[i]; }
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
        else if (osg::DoubleArray* a = dynamic_cast<osg::DoubleArray*>(o))
        { a->resize(count); for (size_t i = 0; i < count; ++i) (*a)[i] = v[i]; }
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
        (void)token;
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

    std::map<std::string, Handler> _handlers;
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
