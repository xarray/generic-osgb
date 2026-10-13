// vsg_integration: the VulkanSceneGraph adapter.
//
// The zero-dependency generic_wrappers reader parses the .osgb/.osgt file into a
// neutral SceneIR (object creation, every serializer property and every explicit
// command). This translation unit only depends on the VSG SDK and on the
// osg-free SceneIR.h, and replays the IR into VSG's default scene graph:
//
//   osg::Group/Geode/Node      -> vsg::Group
//   osg::MatrixTransform ...   -> vsg::MatrixTransform
//   osg::LOD / osg::Switch     -> vsg::LOD / vsg::Switch
//   osg::Geometry              -> vsg::Group holding vertex/index draws
//   osg::*Array                -> vsg::vec3Array / vsg::vec2Array / ...
//   osg::Image                 -> vsg::Data loaded through vsgXchange
//                                 (SGI .rgb is decoded locally: vsgXchange
//                                  has no reader for that OSG format)
//   osg::StateSet/Texture/Material -> pipeline textures / Phong material
//
// Draw nodes are built in finish(), once every array / primitive / state of a
// geometry has been recorded, so the IR's read order no longer matters.
//
// Build: -DENABLE_VSG=ON -DVSG_ROOT=<sdk>
// Usage: vsg_integration <file.osgt|file.osgb> [--list]

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <cstring>
#include <string>

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

    bool has(const std::string& s, const char* sub) { return s.find(sub) != std::string::npos; }

    unsigned int be16(const unsigned char* p) { return ((unsigned)p[0] << 8) | (unsigned)p[1]; }
    unsigned int be32(const unsigned char* p)
    { return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | (unsigned)p[3]; }

    // vsgXchange has no reader for the classic SGI image format (.rgb/.sgi/
    // .rgba), which several osg assets rely on (Images/reflect.rgb, lz.rgb,
    // tank.rgb, ...). Decode the uncompressed and RLE variants into RGBA.
    vsg::ref_ptr<vsg::Data> readSGI(const vsg::Path& filename)
    {
        std::ifstream in(filename, std::ios::binary);
        if (!in) return vsg::ref_ptr<vsg::Data>();

        unsigned char header[512];
        in.read(reinterpret_cast<char*>(header), sizeof(header));
        if (in.gcount() != (std::streamsize)sizeof(header)) return vsg::ref_ptr<vsg::Data>();
        if (be16(header) != 0x01DA) return vsg::ref_ptr<vsg::Data>();   // SGI magic

        int storage = header[2];
        int bpc = header[3];
        unsigned int xsize = be16(header + 6), ysize = be16(header + 8), zsize = be16(header + 10);
        if (bpc != 1 || xsize == 0 || ysize == 0) return vsg::ref_ptr<vsg::Data>();
        if (zsize != 1 && zsize != 3 && zsize != 4) return vsg::ref_ptr<vsg::Data>();

        const size_t channelBytes = (size_t)xsize * ysize;
        std::vector<std::vector<unsigned char> > channels(zsize);
        if (storage == 1)                                        // RLE
        {
            std::vector<std::vector<unsigned int> > offsets(zsize), lengths(zsize);
            for (unsigned int z = 0; z < zsize; ++z)
            {
                offsets[z].resize(ysize); lengths[z].resize(ysize);
                for (unsigned int y = 0; y < ysize; ++y)
                {
                    unsigned char e[8];
                    in.read(reinterpret_cast<char*>(e), 8);
                    offsets[z][y] = be32(e); lengths[z][y] = be32(e + 4);
                }
            }
            for (unsigned int z = 0; z < zsize; ++z)
            {
                channels[z].assign(channelBytes, 0);
                for (unsigned int y = 0; y < ysize; ++y)
                {
                    in.seekg((std::streamoff)offsets[z][y]);
                    std::vector<unsigned char> row(lengths[z][y]);
                    if (!row.empty())
                        in.read(reinterpret_cast<char*>(&row[0]), (std::streamsize)row.size());

                    unsigned char* dst = &channels[z][(size_t)y * xsize];
                    unsigned int x = 0; size_t i = 0;
                    while (x < xsize && i < row.size())
                    {
                        unsigned char b = row[i++];
                        unsigned int count = b & 0x7F;
                        if (count == 0) break;
                        if (b & 0x80)                                 // run of one value
                        {
                            unsigned char v = (i < row.size()) ? row[i++] : 0;
                            for (unsigned int k = 0; k < count && x < xsize; ++k) dst[x++] = v;
                        }
                        else                                          // literal bytes
                            for (unsigned int k = 0; k < count && x < xsize && i < row.size(); ++k)
                                dst[x++] = row[i++];
                    }
                }
            }
        }
        else                                                     // uncompressed
        {
            for (unsigned int z = 0; z < zsize; ++z)
            {
                channels[z].resize(channelBytes);
                in.read(reinterpret_cast<char*>(&channels[z][0]), (std::streamsize)channelBytes);
            }
        }

        // SGI stores rows bottom-to-top; flip to the top-left origin used by the
        // other (vsgXchange) image paths.
        vsg::ref_ptr<vsg::ubvec4Array2D> image = vsg::ubvec4Array2D::create(
            xsize, ysize, vsg::Data::Properties{VK_FORMAT_R8G8B8A8_SRGB});
        for (unsigned int y = 0; y < ysize; ++y)
        {
            const size_t srcRow = (size_t)(ysize - 1 - y) * xsize;
            for (unsigned int x = 0; x < xsize; ++x)
            {
                unsigned char r = channels[0][srcRow + x];
                unsigned char g = (zsize >= 3) ? channels[1][srcRow + x] : r;
                unsigned char b = (zsize >= 3) ? channels[2][srcRow + x] : r;
                unsigned char a = (zsize == 4) ? channels[3][srcRow + x] : 255;
                image->set(x, y, vsg::ubvec4(r, g, b, a));
            }
        }
        return image;
    }


    // GL primitive mode -> the Vulkan *list* topology its expanded indices are
    // drawn with. Every mode is decomposed into a plain list, so the topology is
    // always the matching *_LIST (never STRIP/FAN).
    VkPrimitiveTopology listTopology(int glMode)
    {
        switch (glMode)
        {
        case 0x0000: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;       // GL_POINTS
        case 0x0001:                                                 // GL_LINES
        case 0x0002:                                                 // GL_LINE_LOOP
        case 0x0003: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;        // GL_LINE_STRIP
        default:     return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;    // all triangle modes
        }
    }
}

class VsgSceneBuilder : public osgVerse::ISceneBuilder
{
public:
    explicit VsgSceneBuilder(vsg::ref_ptr<vsg::Options> options)
        : _options(options), _shaderSet(vsg::createPhongShaderSet(options))
    {
        registerHandlers();
    }

    void onObject(const osgVerse::IRObject& obj, int index) override
    {
        Kind kind = classify(obj.className);
        vsg::ref_ptr<vsg::Object> o;

        switch (kind)
        {
        case K_TRANSFORM: o = vsg::MatrixTransform::create(); break;
        case K_LOD:       o = vsg::LOD::create(); break;
        case K_SWITCH:    o = vsg::Switch::create(); break;
        case K_GROUP:
        case K_GEOMETRY:
        case K_GENERIC:   o = vsg::Group::create(); break;
        case K_IMAGE:     o = loadImage(obj); break;
        case K_ARRAY:     o = makeArray(obj); break;
        default:          o = vsg::Object::create(); break;
        }

        while ((int)_objs.size() < index) _objs.push_back(vsg::ref_ptr<vsg::Object>());
        _objs.push_back(o);
        ++_created;

        if (!obj.id.empty()) _byKey[nsFor(obj.className) + "#" + obj.id] = o;
        if (index == 0) _root = o;

        if (vsg::Data* d = dynamic_cast<vsg::Data*>(o.get())) _data[index] = d;

        if (kind == K_GEOMETRY)
        {
            GeomState g;
            g.group = o.cast<vsg::Group>();
            _geoms[index] = g;
        }
        else if (kind == K_PRIMITIVE)
        {
            PrimInfo p;
            p.mode = obj.primMode;
            p.first = obj.primFirst;
            p.count = obj.primCount;
            p.indices = obj.primIndices;
            p.lengths = obj.primLengths;
            if (!obj.vectorData.empty())                 // modern DrawElements indices
            {
                p.indices.resize(obj.vectorData.size());
                for (size_t i = 0; i < obj.vectorData.size(); ++i)
                    p.indices[i] = (unsigned int)obj.vectorData[i];
            }
            _prims[index] = p;
        }
        else if (kind == K_STATESET)
            _stateSet.insert(index);
        else if (kind == K_TEXTURE)
            _texture.insert(index);
        else if (kind == K_MATERIAL)
            _materials[index] = vsg::PhongMaterialValue::create();
    }

    void onCommand(const osgVerse::IRObject& obj, int index,
                   const osgVerse::IRCommand& cmd) override
    {
        vsg::Object* target = (index >= 0 && index < (int)_objs.size()) ? _objs[index].get() : NULL;
        if (!target) return;
        dispatch(target, index, cmd.name, cmd.args);
    }

    void finish() override
    {
        for (std::map<int, GeomState>::iterator it = _geoms.begin(); it != _geoms.end(); ++it)
            buildGeometry(it->first, it->second);
    }

    vsg::ref_ptr<vsg::Node> root() const { return _root.cast<vsg::Node>(); }
    unsigned int created() const { return _created; }
    unsigned int draws() const { return _draws; }
    unsigned int failedImages() const { return _failedImages; }
    const std::map<std::string, unsigned int>& unhandled() const { return _unhandled; }

protected:
    typedef std::vector<osgVerse::IRArg> Args;

    enum Kind { K_GENERIC, K_GROUP, K_TRANSFORM, K_LOD, K_SWITCH,
                K_GEOMETRY, K_ARRAY, K_PRIMITIVE, K_IMAGE, K_STATESET,
                K_TEXTURE, K_MATERIAL };

    struct PrimInfo
    {
        int mode = 0;
        int first = 0;
        unsigned int count = 0;
        std::vector<unsigned int> indices;   // DrawElements
        std::vector<unsigned int> lengths;   // DrawArrayLengths (per-run vertex counts)
    };

    struct GeomState
    {
        vsg::ref_ptr<vsg::Group> group;
        vsg::ref_ptr<vsg::Data> vertex, normal, color, texCoord;
        int stateSet = -1;
        std::vector<int> primitives;
    };

    static Kind classify(const std::string& c)
    {
        if (c == "osg::Geometry") return K_GEOMETRY;
        if (has(c, "LOD")) return K_LOD;
        if (c == "osg::Switch") return K_SWITCH;
        // Note: the primitive check must come before the array check, as the
        // primitive class names also contain "Array" (DrawArrays, DrawArrayLengths).
        if (has(c, "DrawArray") || has(c, "DrawElements") || c == "osg::PrimitiveSet")
            return K_PRIMITIVE;
        if (has(c, "Array")) return K_ARRAY;
        if (c == "osg::Image") return K_IMAGE;
        if (c == "osg::StateSet") return K_STATESET;
        if (has(c, "Texture")) return K_TEXTURE;
        if (c == "osg::Material") return K_MATERIAL;
        if (has(c, "Transform") || c == "osg::Camera") return K_TRANSFORM;
        if (has(c, "Node") || has(c, "Group") || has(c, "Geode") ||
            c == "osg::Billboard" || c == "osg::Sequence") return K_GROUP;
        return K_GENERIC;
    }

    typedef bool (VsgSceneBuilder::*Handler)(vsg::Object*, int, const std::string&, const Args&);

    void on(Handler h, std::initializer_list<const char*> names)
    { for (const char* n : names) _handlers[n] = h; }

    void registerHandlers()
    {
        on(&VsgSceneBuilder::handleAddChild, { "addChild" });
        on(&VsgSceneBuilder::handleAddDrawable, { "addDrawable" });
        on(&VsgSceneBuilder::handleMatrix, { "Matrix" });
        on(&VsgSceneBuilder::handleGeometryArray, { "VertexArray", "NormalArray", "ColorArray",
                                                    "SecondaryColorArray", "FogCoordArray",
                                                    "setVertexArray", "setNormalArray",
                                                    "setColorArray", "setSecondaryColorArray",
                                                    "setFogCoordArray", "setTexCoordArray",
                                                    "setVertexAttribArray" });
        on(&VsgSceneBuilder::handleGeometryList, { "PrimitiveSetList", "TexCoordArrayList",
                                                   "VertexAttribArrayList" });
        on(&VsgSceneBuilder::handlePrimitiveProp, { "Mode", "First", "Count", "NumInstances" });
        on(&VsgSceneBuilder::handleStateSetRef, { "StateSet", "setStateSet" });
        on(&VsgSceneBuilder::handleStateSetProp, { "setTextureAttribute", "setAttribute",
                                                   "setMode", "setTextureMode", "setWrap",
                                                   "setFilter" });
        on(&VsgSceneBuilder::handleTextureImage, { "Image" });
        on(&VsgSceneBuilder::handleTextureProp, { "setWrap", "setFilter", "WrapS", "WrapT", "WrapR",
                                                  "FilterMin", "FilterMag" });
        on(&VsgSceneBuilder::handleMaterial, { "setAmbient", "setDiffuse", "setSpecular",
                                               "setEmission", "setShininess" });
    }

    void dispatch(vsg::Object* o, int index, const std::string& name, const Args& args)
    {
        std::map<std::string, Handler>::const_iterator itr = _handlers.find(name);
        if (itr == _handlers.end() || !(this->*(itr->second))(o, index, name, args))
            ++_unhandled[name];
    }

    // -----------------------------------------------------------------------
    // Handlers
    // -----------------------------------------------------------------------
    bool handleAddChild(vsg::Object* o, int, const std::string&, const Args& args)
    {
        vsg::ref_ptr<vsg::Node> child = resolveNode(args, 0);
        if (!child) return false;
        if (vsg::Group* g = dynamic_cast<vsg::Group*>(o)) { g->addChild(child); return true; }
        if (vsg::LOD* l = dynamic_cast<vsg::LOD*>(o))
        { l->addChild(vsg::LOD::Child{0.0, child}); return true; }
        if (vsg::Switch* s = dynamic_cast<vsg::Switch*>(o))
        { s->addChild(true, child); return true; }
        return false;
    }

    bool handleAddDrawable(vsg::Object* o, int, const std::string&, const Args& args)
    {
        vsg::Group* g = dynamic_cast<vsg::Group*>(o);
        vsg::ref_ptr<vsg::Node> child = resolveNode(args, 0);
        if (!g || !child) return false;
        g->addChild(child);
        return true;
    }

    bool handleMatrix(vsg::Object* o, int, const std::string&, const Args& args)
    {
        vsg::MatrixTransform* mt = dynamic_cast<vsg::MatrixTransform*>(o);
        if (!mt || args.empty() || args[0].kind != osgVerse::IRArg::MATRIX) return false;
        vsg::dmat4 m;
        for (int i = 0; i < 16; ++i) m[i / 4][i % 4] = args[0].v[i];
        mt->matrix = m;
        return true;
    }

    bool handleGeometryArray(vsg::Object* o, int index, const std::string& name, const Args& args)
    {
        if (dynamic_cast<vsg::Group*>(o) == NULL) return false;   // Geometry -> Group
        GeomState& g = _geoms[index];

        size_t ai = 0;
        if (name == "setTexCoordArray" || name == "setVertexAttribArray")
        {
            if (args.size() < 2) return true;
            unsigned int unit = uArg(args, 0);
            if (name == "setVertexAttribArray") return true;   // not used by the Phong shader
            if (unit != 0) return true;
            ai = 1;
        }
        if (args.size() <= ai) return true;

        vsg::ref_ptr<vsg::Data> data = resolveData(args, ai);
        if (name == "VertexArray" || name == "setVertexArray") g.vertex = data;
        else if (name == "NormalArray" || name == "setNormalArray") g.normal = data;
        else if (name == "ColorArray" || name == "setColorArray") g.color = data;
        else if (name == "setTexCoordArray") g.texCoord = data;
        return true;
    }

    bool handleGeometryList(vsg::Object* o, int index, const std::string& name, const Args& args)
    {
        if (dynamic_cast<vsg::Group*>(o) == NULL) return false;
        GeomState& g = _geoms[index];
        if (name == "PrimitiveSetList")
        {
            for (size_t i = 0; i < args.size(); ++i)
            {
                int pi = resolveIndex(args[i]);
                if (pi >= 0) g.primitives.push_back(pi);
            }
        }
        else if (name == "TexCoordArrayList")
        {
            if (!args.empty()) g.texCoord = resolveData(args, 0);
        }
        return true;
    }

    bool handlePrimitiveProp(vsg::Object* o, int index, const std::string& name, const Args& args)
    {
        if (_prims.find(index) == _prims.end()) return false;
        PrimInfo& p = _prims[index];
        if (name == "Mode") p.mode = iArg(args, 0);
        else if (name == "First") p.first = iArg(args, 0);
        else if (name == "Count") p.count = uArg(args, 0);
        return true;
    }

    bool handleStateSetRef(vsg::Object* o, int index, const std::string&, const Args& args)
    {
        int ss = resolveIndex(args, 0);
        if (ss < 0) return true;   // empty reference: nothing to apply
        if (_geoms.count(index)) _geoms[index].stateSet = ss;
        return true;               // group-level state is ignored in this first pass
    }

    bool handleStateSetProp(vsg::Object* o, int index, const std::string& name, const Args& args)
    {
        if (!_stateSet.count(index)) return false;
        if (name == "setTextureAttribute" && args.size() >= 2)
        {
            // A texture unit may also carry non-texture attributes (osg::TexEnv,
            // osg::TexGen, ...); only the osg::Texture* entry defines its image.
            unsigned int unit = uArg(args, 0);
            if (unit == 0 && args[1].kind == osgVerse::IRArg::OBJECT && has(args[1].objType, "Texture"))
            {
                int tex = resolveIndex(args[1]);
                if (tex >= 0) _stateTexture[index] = tex;
            }
        }
        else if (name == "setAttribute" && !args.empty() &&
                 args[0].kind == osgVerse::IRArg::OBJECT && args[0].objType == "osg::Material")
        {
            int attr = resolveIndex(args[0]);
            if (attr >= 0) _stateMaterial[index] = attr;
        }
        return true;   // modes / wraps are not required for the first pass
    }

    bool handleTextureImage(vsg::Object* o, int index, const std::string&, const Args& args)
    {
        if (!_texture.count(index)) return false;
        int image = resolveIndex(args, 0);
        if (image >= 0) _textureImage[index] = image;
        return true;
    }

    bool handleTextureProp(vsg::Object* o, int index, const std::string&, const Args& args)
    {
        return _texture.count(index) != 0;   // wrap/filter recorded but not used yet
    }

    bool handleMaterial(vsg::Object* o, int index, const std::string& name, const Args& args)
    {
        std::map<int, vsg::ref_ptr<vsg::PhongMaterialValue>>::iterator it = _materials.find(index);
        if (it == _materials.end() || args.empty()) return false;
        vsg::PhongMaterial& m = it->second->value();
        if (name == "setShininess") { m.shininess = (float)dArg(args, args.size() - 1); return true; }

        // setAmbient/Diffuse/Specular/Emission: (face, vec4) or (vec4)
        const osgVerse::IRArg& c = args[args.size() - 1];
        if (c.kind != osgVerse::IRArg::VEC4 && c.n < 4) return false;
        vsg::vec4 col((float)c.v[0], (float)c.v[1], (float)c.v[2], (float)c.v[3]);
        if (name == "setAmbient") m.ambient = col;
        else if (name == "setDiffuse") m.diffuse = col;
        else if (name == "setSpecular") m.specular = col;
        else m.emissive = col;
        return true;
    }

    // -----------------------------------------------------------------------
    // Draw construction
    // -----------------------------------------------------------------------
    void buildGeometry(int index, GeomState& g)
    {
        if (!g.group.valid() || !g.vertex.valid()) return;

        // The material of the geometry's state set (optional).
        vsg::ref_ptr<vsg::PhongMaterialValue> material;
        vsg::ref_ptr<vsg::Data> texture;
        if (g.stateSet >= 0)
        {
            std::map<int, int>::iterator tit = _stateTexture.find(g.stateSet);
            if (tit != _stateTexture.end())
            {
                std::map<int, int>::iterator iit = _textureImage.find(tit->second);
                if (iit != _textureImage.end())
                {
                    std::map<int, vsg::ref_ptr<vsg::Data>>::iterator dit = _data.find(iit->second);
                    if (dit != _data.end()) texture = dit->second;
                }
            }
        }
        for (std::map<int, vsg::ref_ptr<vsg::PhongMaterialValue>>::iterator it = _materials.begin();
             it != _materials.end(); ++it)
        {
            // Prefer the material referenced by the geometry's state set.
            std::map<int, int>::iterator sit = _stateMaterial.find(g.stateSet);
            if (sit != _stateMaterial.end()) material = _materials[sit->second];
            else material = it->second;
            break;
        }

        // Expand every primitive set into explicit list indices, grouped by the
        // list topology it needs (a geometry's sets may mix triangles and lines).
        // Everything is expanded to plain lists, so the pipeline topology must be
        // the matching *_LIST (declaring STRIP/FAN here would re-interpret the
        // already-expanded indices and tear the mesh apart).
        std::map<VkPrimitiveTopology, std::vector<uint32_t> > groups;
        if (g.primitives.empty())
        {
            std::vector<uint32_t>& list = groups[VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST];
            size_t n = g.vertex->valueCount();
            list.resize(n);
            for (size_t i = 0; i < n; ++i) list[i] = (uint32_t)i;
        }
        else
        {
            for (size_t i = 0; i < g.primitives.size(); ++i)
            {
                std::map<int, PrimInfo>::iterator it = _prims.find(g.primitives[i]);
                if (it == _prims.end()) continue;
                appendPrimitive(it->second, groups);
            }
        }

        for (std::map<VkPrimitiveTopology, std::vector<uint32_t> >::iterator it = groups.begin();
             it != groups.end(); ++it)
        {
            if (!it->second.empty()) addDraw(g, it->second, it->first, material, texture);
        }
    }

    // Expand one primitive set into explicit list indices.
    static void appendPrimitive(const PrimInfo& p,
                                std::map<VkPrimitiveTopology, std::vector<uint32_t> >& groups)
    {
        std::vector<uint32_t>& out = groups[listTopology(p.mode)];
        if (!p.lengths.empty())
        {
            // DrawArrayLengths: every length starts a new run of the mode, so the
            // runs must be expanded independently (concatenating them into one
            // long strip/fan would web the separate runs together).
            uint32_t offset = (uint32_t)p.first;
            for (size_t k = 0; k < p.lengths.size(); ++k)
            {
                appendRun(p.mode, offset, p.lengths[k], out);
                offset += p.lengths[k];
            }
        }
        else if (!p.indices.empty())
            expand(p.mode, p.indices, out);
        else
            appendRun(p.mode, (uint32_t)p.first, p.count, out);
    }

    static void appendRun(int mode, uint32_t first, uint32_t count, std::vector<uint32_t>& out)
    {
        std::vector<uint32_t> src(count);
        for (uint32_t i = 0; i < count; ++i) src[i] = first + i;
        expand(mode, src, out);
    }

    // Expand a vertex/index sequence of `mode` into a plain triangle/line/point
    // list (strips, fans, quads and line loops are decomposed).
    static void expand(int mode, const std::vector<uint32_t>& src, std::vector<uint32_t>& out)
    {
        switch (mode)
        {
        case 0x0005:   // GL_TRIANGLE_STRIP
            for (size_t i = 2; i < src.size(); ++i)
                addTriangle(out, src[i-2], src[i-1], src[i], (i & 1) != 0);
            break;
        case 0x0006:   // GL_TRIANGLE_FAN
        case 0x0009:   // GL_POLYGON
            for (size_t i = 2; i < src.size(); ++i)
                addTriangle(out, src[0], src[i-1], src[i], false);
            break;
        case 0x0007:   // GL_QUADS
            for (size_t i = 0; i + 4 <= src.size(); i += 4)
            {
                addTriangle(out, src[i+0], src[i+1], src[i+2], false);
                addTriangle(out, src[i+0], src[i+2], src[i+3], false);
            }
            break;
        case 0x0008:   // GL_QUAD_STRIP
            for (size_t i = 0; i + 4 <= src.size(); i += 2)
            {
                addTriangle(out, src[i+0], src[i+1], src[i+2], false);
                addTriangle(out, src[i+1], src[i+3], src[i+2], false);
            }
            break;
        case 0x0002:   // GL_LINE_LOOP
            for (size_t i = 0; i < src.size(); ++i)
            { out.push_back(src[i]); out.push_back(src[(i + 1) % src.size()]); }
            break;
        case 0x0003:   // GL_LINE_STRIP
            for (size_t i = 0; i + 1 < src.size(); ++i)
            { out.push_back(src[i]); out.push_back(src[i+1]); }
            break;
        default:       // GL_POINTS / GL_LINES / GL_TRIANGLES (already a list)
            out.insert(out.end(), src.begin(), src.end());
        }
    }

    static void addTriangle(std::vector<uint32_t>& out, uint32_t a, uint32_t b, uint32_t c, bool swap)
    {
        out.push_back(a);
        out.push_back(swap ? c : b);
        out.push_back(swap ? b : c);
    }

    void addDraw(GeomState& g, const std::vector<uint32_t>& indices, VkPrimitiveTopology topology,
                 vsg::ref_ptr<vsg::PhongMaterialValue> material, vsg::ref_ptr<vsg::Data> texture)
    {
        vsg::ref_ptr<vsg::GraphicsPipelineConfigurator> config =
            vsg::GraphicsPipelineConfigurator::create(_shaderSet);

        vsg::DataList arrays;
        size_t vCount = g.vertex->valueCount();
        vsg::ref_ptr<vsg::Data> normal = perVertex(g.normal, vCount);
        vsg::ref_ptr<vsg::Data> color = perVertex(g.color, vCount);
        vsg::ref_ptr<vsg::Data> texCoord = perVertex(g.texCoord, vCount);

        config->assignArray(arrays, "vsg_Vertex", VK_VERTEX_INPUT_RATE_VERTEX, g.vertex);
        config->assignArray(arrays, "vsg_Normal", VK_VERTEX_INPUT_RATE_VERTEX,
                            normal.valid() ? normal : defaultVec3Array(vCount));
        config->assignArray(arrays, "vsg_Color", VK_VERTEX_INPUT_RATE_VERTEX,
                            color.valid() ? color : defaultVec4Array(vCount));
        if (texCoord.valid())
            config->assignArray(arrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX, texCoord);
        if (texture.valid())
            config->assignTexture("diffuseMap", texture);

        // The Phong shader reads its material from the MATERIAL descriptor set.
        // Always bind it: files without an osg::Material would otherwise get an
        // unbound (all zero) material and render black.
        config->assignDescriptor("material", material.valid() ? material : vsg::PhongMaterialValue::create());
        config->assignDescriptor("texCoordIndices", vsg::TexCoordIndicesValue::create());

        // primitive topology (input assembly state)
        for (auto& ps : config->pipelineStates)
        {
            if (vsg::InputAssemblyState* ias = dynamic_cast<vsg::InputAssemblyState*>(ps.get()))
                ias->topology = topology;
            else if (vsg::RasterizationState* rs = dynamic_cast<vsg::RasterizationState*>(ps.get()))
                rs->cullMode = VK_CULL_MODE_NONE;   // osg assets are not winding-consistent
        }

        config->init();

        vsg::ref_ptr<vsg::StateGroup> stateGroup = vsg::StateGroup::create();
        config->copyTo(stateGroup);

        vsg::ref_ptr<vsg::uintArray> indexArray = vsg::uintArray::create(indices.size());
        for (size_t i = 0; i < indices.size(); ++i) indexArray->set(i, indices[i]);

        vsg::ref_ptr<vsg::VertexIndexDraw> draw = vsg::VertexIndexDraw::create();
        draw->assignArrays(arrays);
        draw->assignIndices(indexArray);
        draw->indexCount = (uint32_t)indexArray->size();
        draw->instanceCount = 1;
        stateGroup->addChild(draw);

        g.group->addChild(stateGroup);
        ++_draws;
    }

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    // An osg array may be BIND_OVERALL (a single element shared by every vertex).
    // Per-vertex shader inputs need one element per vertex, so expand a
    // single-element array; reject any other size mismatch.
    static vsg::ref_ptr<vsg::Data> perVertex(vsg::ref_ptr<vsg::Data> a, size_t n)
    {
        if (!a) return vsg::ref_ptr<vsg::Data>();
        if (a->valueCount() == n) return a;
        if (a->valueCount() != 1 || n == 0) return vsg::ref_ptr<vsg::Data>();

        if (vsg::vec4Array* v = dynamic_cast<vsg::vec4Array*>(a.get()))
        { vsg::vec4 c = *v->begin(); vsg::ref_ptr<vsg::vec4Array> r = vsg::vec4Array::create(n); for (size_t i = 0; i < n; ++i) r->set(i, c); return r; }
        if (vsg::vec3Array* v = dynamic_cast<vsg::vec3Array*>(a.get()))
        { vsg::vec3 c = *v->begin(); vsg::ref_ptr<vsg::vec3Array> r = vsg::vec3Array::create(n); for (size_t i = 0; i < n; ++i) r->set(i, c); return r; }
        if (vsg::vec2Array* v = dynamic_cast<vsg::vec2Array*>(a.get()))
        { vsg::vec2 c = *v->begin(); vsg::ref_ptr<vsg::vec2Array> r = vsg::vec2Array::create(n); for (size_t i = 0; i < n; ++i) r->set(i, c); return r; }
        return vsg::ref_ptr<vsg::Data>();
    }

    static vsg::ref_ptr<vsg::Data> defaultVec3Array(size_t n)
    {
        vsg::ref_ptr<vsg::vec3Array> a = vsg::vec3Array::create(n);
        for (size_t i = 0; i < n; ++i) a->set(i, vsg::vec3(0.0f, 0.0f, 1.0f));
        return a;
    }

    static vsg::ref_ptr<vsg::Data> defaultVec4Array(size_t n)
    {
        vsg::ref_ptr<vsg::vec4Array> a = vsg::vec4Array::create(n);
        for (size_t i = 0; i < n; ++i) a->set(i, vsg::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        return a;
    }

    vsg::ref_ptr<vsg::Object> makeArray(const osgVerse::IRObject& obj)
    {
        std::string c = obj.className;
        std::vector<double> v = obj.vectorData;
        size_t n = v.size();
        int comp = obj.vectorComponents;

        if (c == "osg::Vec2Array") { vsg::ref_ptr<vsg::vec2Array> a = vsg::vec2Array::create(n / 2); for (size_t i = 0; i < n / 2; ++i) a->set(i, vsg::vec2((float)v[2*i], (float)v[2*i+1])); return a; }
        if (c == "osg::Vec3Array") { vsg::ref_ptr<vsg::vec3Array> a = vsg::vec3Array::create(n / 3); for (size_t i = 0; i < n / 3; ++i) a->set(i, vsg::vec3((float)v[3*i], (float)v[3*i+1], (float)v[3*i+2])); return a; }
        if (c == "osg::Vec4Array") { vsg::ref_ptr<vsg::vec4Array> a = vsg::vec4Array::create(n / 4); for (size_t i = 0; i < n / 4; ++i) a->set(i, vsg::vec4((float)v[4*i], (float)v[4*i+1], (float)v[4*i+2], (float)v[4*i+3])); return a; }
        if (c == "osg::Vec2dArray") { vsg::ref_ptr<vsg::dvec2Array> a = vsg::dvec2Array::create(n / 2); for (size_t i = 0; i < n / 2; ++i) a->set(i, vsg::dvec2(v[2*i], v[2*i+1])); return a; }
        if (c == "osg::Vec3dArray") { vsg::ref_ptr<vsg::dvec3Array> a = vsg::dvec3Array::create(n / 3); for (size_t i = 0; i < n / 3; ++i) a->set(i, vsg::dvec3(v[3*i], v[3*i+1], v[3*i+2])); return a; }
        if (c == "osg::Vec4dArray") { vsg::ref_ptr<vsg::dvec4Array> a = vsg::dvec4Array::create(n / 4); for (size_t i = 0; i < n / 4; ++i) a->set(i, vsg::dvec4(v[4*i], v[4*i+1], v[4*i+2], v[4*i+3])); return a; }
        if (c == "osg::FloatArray") { vsg::ref_ptr<vsg::floatArray> a = vsg::floatArray::create(n); for (size_t i = 0; i < n; ++i) a->set(i, (float)v[i]); return a; }
        if (c == "osg::DoubleArray") { vsg::ref_ptr<vsg::doubleArray> a = vsg::doubleArray::create(n); for (size_t i = 0; i < n; ++i) a->set(i, v[i]); return a; }
        if (c == "osg::UIntArray") { vsg::ref_ptr<vsg::uintArray> a = vsg::uintArray::create(n); for (size_t i = 0; i < n; ++i) a->set(i, (uint32_t)v[i]); return a; }
        if (c == "osg::IntArray") { vsg::ref_ptr<vsg::intArray> a = vsg::intArray::create(n); for (size_t i = 0; i < n; ++i) a->set(i, (int32_t)v[i]); return a; }
        if (c == "osg::UShortArray") { vsg::ref_ptr<vsg::ushortArray> a = vsg::ushortArray::create(n); for (size_t i = 0; i < n; ++i) a->set(i, (uint16_t)v[i]); return a; }
        if (c == "osg::UByteArray") { vsg::ref_ptr<vsg::ubyteArray> a = vsg::ubyteArray::create(n); for (size_t i = 0; i < n; ++i) a->set(i, (uint8_t)v[i]); return a; }

        (void)comp;
        return vsg::Object::create();   // unsupported array type: keep an IR placeholder
    }

    vsg::ref_ptr<vsg::Object> loadImage(const osgVerse::IRObject& obj)
    {
        if (obj.fileName.empty()) return vsg::Object::create();
        std::string path = obj.fileName;
        for (size_t i = 0; i < path.size(); ++i) if (path[i] == '\\') path[i] = '/';

        vsg::Path found = vsg::findFile(vsg::Path(path), _options);
        if (found)
        {
            try
            {
                if (vsg::ref_ptr<vsg::Data> data = vsg::read_cast<vsg::Data>(found, _options))
                    return data;
            }
            catch (const std::exception&) {}

            // vsgXchange has no SGI (.rgb) reader; decode it ourselves.
            if (vsg::ref_ptr<vsg::Data> sgi = readSGI(found)) return sgi;
        }
        ++_failedImages;
        return vsg::Object::create();
    }

    int resolveIndex(const osgVerse::IRArg& a) const
    {
        if (a.kind != osgVerse::IRArg::OBJECT || a.objId.empty()) return -1;
        std::map<std::string, vsg::ref_ptr<vsg::Object>>::const_iterator itr =
            _byKey.find(nsFor(a.objType) + "#" + a.objId);
        if (itr == _byKey.end() || !itr->second) return -1;
        for (size_t i = 0; i < _objs.size(); ++i) if (_objs[i] == itr->second) return (int)i;
        return -1;
    }

    int resolveIndex(const Args& a, size_t i) const
    { static osgVerse::IRArg empty; return i < a.size() ? resolveIndex(a[i]) : -1; }

    vsg::ref_ptr<vsg::Node> resolveNode(const Args& a, size_t i) const
    {
        int idx = resolveIndex(a, i);
        return (idx >= 0) ? _objs[idx].cast<vsg::Node>() : vsg::ref_ptr<vsg::Node>();
    }

    vsg::ref_ptr<vsg::Data> resolveData(const Args& a, size_t i) const
    {
        int idx = resolveIndex(a, i);
        if (idx < 0) return vsg::ref_ptr<vsg::Data>();
        std::map<int, vsg::ref_ptr<vsg::Data>>::const_iterator itr = _data.find(idx);
        return itr != _data.end() ? itr->second : vsg::ref_ptr<vsg::Data>();
    }

    static const osgVerse::IRArg& arg(const Args& a, size_t i)
    { static osgVerse::IRArg empty; return i < a.size() ? a[i] : empty; }
    static unsigned int uArg(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (unsigned int)(v.kind == osgVerse::IRArg::INT ? v.i : v.u); }
    static int iArg(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return (int)(v.kind == osgVerse::IRArg::INT ? v.i : (long long)v.u); }
    static double dArg(const Args& a, size_t i)
    { const osgVerse::IRArg& v = arg(a, i); return v.kind == osgVerse::IRArg::DOUBLE ? v.d : (double)v.i; }

    vsg::ref_ptr<vsg::Options> _options;
    vsg::ref_ptr<vsg::ShaderSet> _shaderSet;

    std::map<std::string, Handler> _handlers;
    std::vector<vsg::ref_ptr<vsg::Object>> _objs;
    std::map<std::string, vsg::ref_ptr<vsg::Object>> _byKey;
    std::map<int, vsg::ref_ptr<vsg::Data>> _data;
    std::map<int, PrimInfo> _prims;
    std::map<int, GeomState> _geoms;
    std::set<int> _stateSet, _texture;
    std::map<int, int> _stateTexture, _textureImage, _stateMaterial;
    std::map<int, vsg::ref_ptr<vsg::PhongMaterialValue>> _materials;
    vsg::ref_ptr<vsg::Object> _root;
    unsigned int _created = 0, _failedImages = 0, _draws = 0;
    std::map<std::string, unsigned int> _unhandled;
};

// Capture the most recently presented frame into an image file. Useful for
// headless/CI verification and for grabbing a still from the viewer.
static bool saveScreenshot(vsg::ref_ptr<vsg::Window> window, const vsg::Path& filename,
                           vsg::ref_ptr<vsg::Options> options)
{
    uint32_t width = window->extent2D().width;
    uint32_t height = window->extent2D().height;
    auto device = window->getDevice();
    auto physicalDevice = window->getPhysicalDevice();
    auto swapchain = window->getSwapchain();

    // image index (1) is the frame presented one frame ago (already rendered)
    auto sourceImage = window->imageView(window->imageIndex(1))->image;
    VkFormat sourceFormat = swapchain->getImageFormat();

    VkFormatProperties srcProperties, dstProperties;
    vkGetPhysicalDeviceFormatProperties(*physicalDevice, sourceFormat, &srcProperties);
    vkGetPhysicalDeviceFormatProperties(*physicalDevice, VK_FORMAT_R8G8B8A8_SRGB, &dstProperties);

    bool supportsBlit = ((srcProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0) &&
                        ((dstProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0);
    VkFormat targetFormat = supportsBlit ? VK_FORMAT_R8G8B8A8_SRGB : sourceFormat;

    vsg::ref_ptr<vsg::Image> destinationImage = vsg::Image::create();
    destinationImage->imageType = VK_IMAGE_TYPE_2D;
    destinationImage->format = targetFormat;
    destinationImage->extent.width = width;
    destinationImage->extent.height = height;
    destinationImage->extent.depth = 1;
    destinationImage->arrayLayers = 1;
    destinationImage->mipLevels = 1;
    destinationImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    destinationImage->samples = VK_SAMPLE_COUNT_1_BIT;
    destinationImage->tiling = VK_IMAGE_TILING_LINEAR;
    destinationImage->usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    destinationImage->compile(device);

    vsg::ref_ptr<vsg::DeviceMemory> deviceMemory = vsg::DeviceMemory::create(
        device, destinationImage->getMemoryRequirements(device->deviceID),
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    destinationImage->bind(deviceMemory, 0);

    vsg::ref_ptr<vsg::Commands> commands = vsg::Commands::create();
    commands->addChild(vsg::PipelineBarrier::create(
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        vsg::ImageMemoryBarrier::create(0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, destinationImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}),
        vsg::ImageMemoryBarrier::create(VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, sourceImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})));

    if (supportsBlit)
    {
        VkImageBlit region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = 1;
        region.srcOffsets[1] = VkOffset3D{(int32_t)width, (int32_t)height, 1};
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.dstOffsets[1] = VkOffset3D{(int32_t)width, (int32_t)height, 1};

        vsg::ref_ptr<vsg::BlitImage> blitImage = vsg::BlitImage::create();
        blitImage->srcImage = sourceImage;
        blitImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        blitImage->dstImage = destinationImage;
        blitImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        blitImage->regions.push_back(region);
        blitImage->filter = VK_FILTER_NEAREST;
        commands->addChild(blitImage);
    }
    else
    {
        VkImageCopy region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.extent = VkExtent3D{width, height, 1};

        vsg::ref_ptr<vsg::CopyImage> copyImage = vsg::CopyImage::create();
        copyImage->srcImage = sourceImage;
        copyImage->srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        copyImage->dstImage = destinationImage;
        copyImage->dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        copyImage->regions.push_back(region);
        commands->addChild(copyImage);
    }

    commands->addChild(vsg::PipelineBarrier::create(
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        vsg::ImageMemoryBarrier::create(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, destinationImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}),
        vsg::ImageMemoryBarrier::create(VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, sourceImage,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1})));

    vsg::ref_ptr<vsg::Fence> fence = vsg::Fence::create(device);
    uint32_t queueFamilyIndex = physicalDevice->getQueueFamily(VK_QUEUE_GRAPHICS_BIT);
    vsg::ref_ptr<vsg::CommandPool> commandPool = vsg::CommandPool::create(device, queueFamilyIndex);
    vsg::ref_ptr<vsg::Queue> queue = device->getQueue(queueFamilyIndex);
    vsg::submitCommandsToQueue(commandPool, fence, 100000000000, queue,
        [&](vsg::CommandBuffer& commandBuffer) { commands->record(commandBuffer); });

    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(*device, destinationImage->vk(device->deviceID), &subResource, &subResourceLayout);

    size_t destRowWidth = width * sizeof(vsg::ubvec4);
    vsg::ref_ptr<vsg::Data> imageData;
    if (destRowWidth == subResourceLayout.rowPitch)
    {
        imageData = vsg::MappedData<vsg::ubvec4Array2D>::create(
            deviceMemory, subResourceLayout.offset, 0, vsg::Data::Properties{targetFormat}, width, height);
    }
    else
    {
        vsg::ref_ptr<vsg::ubyteArray> mappedData = vsg::MappedData<vsg::ubyteArray>::create(
            deviceMemory, subResourceLayout.offset, 0, vsg::Data::Properties{targetFormat},
            subResourceLayout.rowPitch * height);
        imageData = vsg::ubvec4Array2D::create(width, height, vsg::Data::Properties{targetFormat});
        for (uint32_t row = 0; row < height; ++row)
            std::memcpy(imageData->dataPointer(row * width),
                        mappedData->dataPointer(row * subResourceLayout.rowPitch), destRowWidth);
    }
    return vsg::write(imageData, filename, options);
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: vsg_integration <file.osgt|file.osgb> [--list] [--screenshot <image>]" << std::endl;
        return 1;
    }
    const std::string filename = argv[1];
    bool listOnly = false;
    std::string screenshotFile;
    for (int i = 2; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--list") listOnly = true;
        else if (a == "--screenshot" && i + 1 < argc) screenshotFile = argv[++i];
    }

    // Let relative texture/image names in the asset resolve.
    std::string dir = filename;
    std::string::size_type s = dir.find_last_of("/\\");
    dir = (s == std::string::npos) ? "." : dir.substr(0, s);

    vsg::ref_ptr<vsg::Options> options = vsg::Options::create();
    options->add(vsgXchange::all::create());
    options->paths.push_back(dir);

    osgVerse::SceneIR ir;
    if (!osgVerse::readSceneIR(filename, ir))
    {
        std::cerr << "Failed to read " << filename << std::endl;
        return 1;
    }

    VsgSceneBuilder builder(options);
    osgVerse::buildScene(ir, builder);

    std::cout << "IR objects  : " << ir.objects.size() << " (" << ir.events.size() << " events)" << std::endl;
    std::cout << "Rebuilt     : " << builder.created() << " vsg objects" << std::endl;
    std::cout << "Draws       : " << builder.draws() << std::endl;
    if (builder.failedImages())
        std::cout << "Missing     : " << builder.failedImages() << " images not found on disk" << std::endl;

    const std::map<std::string, unsigned int>& unh = builder.unhandled();
    if (!unh.empty())
    {
        std::cout << "Unhandled properties/commands (" << unh.size() << "):" << std::endl;
        for (std::map<std::string, unsigned int>::const_iterator it = unh.begin(); it != unh.end(); ++it)
            std::cout << "  " << it->first << " x" << it->second << std::endl;
    }

    vsg::ref_ptr<vsg::Node> root = builder.root();
    if (!root) { std::cerr << "No renderable root rebuilt." << std::endl; return 2; }

    std::cout << "Root        : " << root->className() << std::endl;

    // Frame the scene with a LookAt derived from its bounds.
    vsg::ComputeBounds computeBounds;
    root->accept(computeBounds);
    vsg::dbox bounds = computeBounds.bounds;
    if (!bounds.valid()) bounds = vsg::dbox(vsg::dvec3(-1.0, -1.0, -1.0), vsg::dvec3(1.0, 1.0, 1.0));
    std::cout << "Bounds      : " << bounds.min << " - " << bounds.max << std::endl;

    if (listOnly) return 0;

    // -----------------------------------------------------------------------
    // Viewer
    // -----------------------------------------------------------------------
    vsg::ref_ptr<vsg::WindowTraits> traits = vsg::WindowTraits::create();
    traits->windowTitle = "generic_osgwrappers - " + filename;
    traits->debugLayer = false;

    vsg::ref_ptr<vsg::Viewer> viewer = vsg::Viewer::create();
    vsg::ref_ptr<vsg::Window> window = vsg::Window::create(traits);
    if (!window)
    {
        std::cerr << "Failed to create a Vulkan window (need a display / GPU)." << std::endl;
        return 3;
    }
    viewer->addWindow(window);

    vsg::dvec3 center = (bounds.min + bounds.max) * 0.5;
    double radius = vsg::length(bounds.max - bounds.min) * 0.5;
    if (radius <= 0.0) radius = 1.0;

    vsg::ref_ptr<vsg::LookAt> lookAt = vsg::LookAt::create(
        center + vsg::dvec3(0.0, -radius * 3.0, radius * 1.5), center, vsg::dvec3(0.0, 0.0, 1.0));

    vsg::ref_ptr<vsg::Perspective> perspective = vsg::Perspective::create(
        30.0, (double)window->extent2D().width / (double)window->extent2D().height,
        radius * 0.01, radius * 20.0);

    vsg::ref_ptr<vsg::Camera> camera = vsg::Camera::create(
        perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // createRenderGraphForView() builds the View, adds a headlight (the osg
    // assets carry no lights of their own, so a Phong pipeline would render
    // black without it) and installs a window resize handler.
    vsg::ref_ptr<vsg::RenderGraph> renderGraph = vsg::createRenderGraphForView(window, camera, root);
    vsg::ref_ptr<vsg::CommandGraph> commandGraph = vsg::CommandGraph::create(window);
    commandGraph->addChild(renderGraph);
    viewer->assignRecordAndSubmitTaskAndPresentation({ commandGraph });

    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));

    viewer->compile();

    uint32_t frameCount = 0;
    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();

        // Give the pipeline a couple of frames, then capture and exit.
        if (!screenshotFile.empty() && ++frameCount >= 3)
        {
            std::cout << (saveScreenshot(window, screenshotFile, options) ?
                          "Screenshot  : " : "Failed to write screenshot: ") << screenshotFile << std::endl;
            break;
        }
    }
    return 0;
}
