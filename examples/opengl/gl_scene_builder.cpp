// gl_scene_builder.cpp -- SceneIR -> OpenGL 3.3 core buffers / node tree.

#include "gl_scene_builder.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

#include "image_decode.h"

namespace gl_ir
{

namespace
{

const double kInfinity = 1.0e30;

bool has(const std::string& s, const char* sub)
{
    return s.find(sub) != std::string::npos;
}

GLenum listMode(int mode)
{
    switch (mode)
    {
    case 0x0000:                                   // GL_POINTS
        return GL_POINTS;
    case 0x0001: case 0x0002: case 0x0003:         // LINES / LINE_LOOP / LINE_STRIP
        return GL_LINES;
    default:                                       // everything else is decomposed
        return GL_TRIANGLES;
    }
}

void addTriangle(std::vector<GLuint>& out, GLuint a, GLuint b, GLuint c, bool swap)
{
    out.push_back(a);
    out.push_back(swap ? c : b);
    out.push_back(swap ? b : c);
}

}  // namespace

// ---------------------------------------------------------------------------
// Mat4 / Bounds
// ---------------------------------------------------------------------------
Mat4::Mat4()
{
    std::memset(m, 0, sizeof(m));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

Mat4 Mat4::fromDoubles(const double* values)
{
    Mat4 result;
    for (int i = 0; i < 16; ++i) result.m[i] = (float)values[i];
    return result;
}

Mat4 Mat4::operator*(const Mat4& other) const
{
    Mat4 result;
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += m[k * 4 + r] * other.m[c * 4 + k];
            result.m[c * 4 + r] = sum;
        }
    return result;
}

void Mat4::transformPoint(const double in[3], double out[3]) const
{
    for (int r = 0; r < 3; ++r)
        out[r] = m[0 * 4 + r] * in[0] + m[1 * 4 + r] * in[1] +
                 m[2 * 4 + r] * in[2] + m[3 * 4 + r];
}

Bounds::Bounds()
{
    for (int i = 0; i < 3; ++i)
    {
        min[i] = std::numeric_limits<double>::max();
        max[i] = -std::numeric_limits<double>::max();
    }
}

bool Bounds::valid() const
{
    return min[0] <= max[0] && min[1] <= max[1] && min[2] <= max[2];
}

void Bounds::expand(const float point[3])
{
    const double values[3] = { point[0], point[1], point[2] };
    expand(values);
}

void Bounds::expand(const double point[3])
{
    for (int i = 0; i < 3; ++i)
    {
        min[i] = std::min(min[i], point[i]);
        max[i] = std::max(max[i], point[i]);
    }
}

void Bounds::expand(const Bounds& other)
{
    if (!other.valid()) return;
    expand(other.min);
    expand(other.max);
}

Bounds Bounds::transformed(const Mat4& matrix) const
{
    Bounds result;
    if (!valid()) return result;

    for (int corner = 0; corner < 8; ++corner)
    {
        double p[3] = {
            (corner & 1) ? max[0] : min[0],
            (corner & 2) ? max[1] : min[1],
            (corner & 4) ? max[2] : min[2]
        };
        double out[3];
        matrix.transformPoint(p, out);
        result.expand(out);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Builder
// ---------------------------------------------------------------------------
GlSceneBuilder::GlSceneBuilder(const std::string& filename)
    : _filename(filename)
{
    std::string::size_type slash = filename.find_last_of("/\\");
    _baseDir = (slash == std::string::npos) ? std::string(".") : filename.substr(0, slash);
    if (_baseDir.empty()) _baseDir = "/";
}

GlSceneBuilder::MaterialData::MaterialData()
{
    ambient[0] = ambient[1] = ambient[2] = 0.2f; ambient[3] = 1.0f;
    diffuse[0] = diffuse[1] = diffuse[2] = 0.8f; diffuse[3] = 1.0f;
    specular[0] = specular[1] = specular[2] = 0.0f; specular[3] = 1.0f;
    emission[0] = emission[1] = emission[2] = 0.0f; emission[3] = 1.0f;
}

GlSceneBuilder::Kind GlSceneBuilder::classify(const std::string& c)
{
    if (c == "osg::Geometry") return K_GEOMETRY;
    if (has(c, "PagedLOD")) return K_PAGEDLOD;
    if (c == "osg::ProxyNode") return K_PROXY;
    if (has(c, "LOD")) return K_LOD;
    if (c == "osg::Switch") return K_SWITCH;
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

std::string GlSceneBuilder::nsFor(const std::string& className)
{
    if (className == "osg::Image") return "I";
    if (className.find("Array") != std::string::npos) return "A";
    if (className.find("PrimitiveSet") != std::string::npos ||
        className.find("DrawArrays") != std::string::npos ||
        className.find("DrawElements") != std::string::npos) return "P";
    return "O";
}

void GlSceneBuilder::onObject(const osgVerse::IRObject& obj, int index)
{
    const Kind kind = classify(obj.className);

    if (!obj.id.empty()) _byKey[nsFor(obj.className) + "#" + obj.id] = index;

    std::shared_ptr<Node> node;
    switch (kind)
    {
    case K_TRANSFORM: node = std::make_shared<Node>(); node->kind = Node::TRANSFORM; break;
    case K_GEOMETRY:  node = std::make_shared<Node>(); node->kind = Node::GEOMETRY; break;
    case K_LOD:       node = std::make_shared<Node>(); node->kind = Node::LOD; break;
    case K_PAGEDLOD:
    case K_PROXY:     node = std::make_shared<Node>(); node->kind = Node::PAGED_LOD; break;
    case K_SWITCH:    node = std::make_shared<Node>(); node->kind = Node::SWITCH; break;
    case K_GROUP:
    case K_GENERIC:   node = std::make_shared<Node>(); node->kind = Node::GROUP; break;
    default: break;
    }

    ++_created;
    if (index == 0) _rootIndex = index;

    if (node)
    {
        _nodes[index] = node;

        if (kind == K_GEOMETRY)
        {
            GeomState state;
            state.node = node;
            _geoms[index] = state;
        }
        else if (kind == K_LOD || kind == K_PAGEDLOD || kind == K_PROXY)
        {
            LodState state;
            state.paged = (kind != K_LOD);
            state.node = node;
            _lods[index] = state;
            if (state.paged) ++_pagedLods;
        }
    }

    if (kind == K_ARRAY)
    {
        ArrayData array;
        array.components = obj.vectorComponents;
        if (array.components <= 0)
        {
            if (has(obj.className, "Vec4")) array.components = 4;
            else if (has(obj.className, "Vec3")) array.components = 3;
            else if (has(obj.className, "Vec2")) array.components = 2;
            else array.components = 1;
        }
        array.normalized = has(obj.className, "ub") || has(obj.className, "UByte") ||
                           has(obj.className, "UChar");
        array.values.reserve(obj.vectorData.size());
        for (size_t i = 0; i < obj.vectorData.size(); ++i)
            array.values.push_back((float)obj.vectorData[i]);
        _arrays[index] = array;
    }
    else if (kind == K_PRIMITIVE)
    {
        PrimData prim;
        prim.mode = obj.primMode;
        prim.first = obj.primFirst;
        prim.count = obj.primCount;
        prim.indices = obj.primIndices;
        prim.lengths = obj.primLengths;
        if (!obj.vectorData.empty())
        {
            prim.indices.resize(obj.vectorData.size());
            for (size_t i = 0; i < obj.vectorData.size(); ++i)
                prim.indices[i] = (unsigned int)obj.vectorData[i];
        }
        _prims[index] = prim;
    }
    else if (kind == K_IMAGE)
    {
        uploadTexture(index, obj);
    }
    else if (kind == K_STATESET)
    {
        _stateSets.insert(index);
    }
    else if (kind == K_TEXTURE)
    {
        _textures.insert(index);
    }
    else if (kind == K_MATERIAL)
    {
        _materials[index] = MaterialData();
    }
}

void GlSceneBuilder::onCommand(const osgVerse::IRObject&, int index,
                               const osgVerse::IRCommand& command)
{
    const std::string& name = command.name;
    const Args& args = command.args;
    bool handled = false;

    std::map<int, std::shared_ptr<Node> >::iterator nit = _nodes.find(index);
    std::shared_ptr<Node> node = (nit != _nodes.end()) ? nit->second : std::shared_ptr<Node>();

    if (name == "addChild" || name == "addDrawable")
    {
        std::shared_ptr<Node> child = nodeFor(args, 0);
        if (node && child)
        {
            std::map<int, LodState>::iterator lit = _lods.find(index);
            if (lit != _lods.end()) lit->second.children.push_back(child);
            else node->children.push_back(child);
            handled = true;
        }
    }
    else if (name == "Matrix")
    {
        if (node && !args.empty() && args[0].kind == osgVerse::IRArg::MATRIX)
        {
            node->matrix = Mat4::fromDoubles(args[0].v);
            handled = true;
        }
    }
    else if (name == "StateSet" || name == "setStateSet")
    {
        std::map<int, GeomState>::iterator git = _geoms.find(index);
        if (git != _geoms.end())
        {
            git->second.stateSet = resolveIndex(args.empty() ? osgVerse::IRArg() : args[0]);
            handled = true;
        }
    }
    else if (name == "VertexArray" || name == "setVertexArray" ||
             name == "NormalArray" || name == "setNormalArray" ||
             name == "ColorArray" || name == "setColorArray" ||
             name == "SecondaryColorArray" || name == "setSecondaryColorArray" ||
             name == "FogCoordArray" || name == "setFogCoordArray" ||
             name == "setTexCoordArray" || name == "setVertexAttribArray")
    {
        if (_geoms.find(index) != _geoms.end())
        {
            handleGeometryArray(args, index, name, handled);
        }
    }
    else if (name == "PrimitiveSetList" || name == "TexCoordArrayList" ||
             name == "VertexAttribArrayList")
    {
        std::map<int, GeomState>::iterator git = _geoms.find(index);
        if (git != _geoms.end())
        {
            if (name == "PrimitiveSetList")
            {
                for (size_t i = 0; i < args.size(); ++i)
                {
                    int prim = resolveIndex(args[i]);
                    if (prim >= 0) git->second.primitives.push_back(prim);
                }
            }
            else if (name == "TexCoordArrayList" && !args.empty())
                git->second.texCoord = resolveIndex(args[0]);
            handled = true;
        }
    }
    else if (name == "Mode" || name == "First" || name == "Count" || name == "NumInstances")
    {
        if (_prims.find(index) != _prims.end())
        {
            handlePrimitiveProp(args, index, name, handled);
        }
    }
    else if (name == "setTextureAttribute" && args.size() >= 2)
    {
        if (_stateSets.count(index))
        {
            const unsigned int unit = uArg(args, 0);
            if (unit == 0 && args[1].kind == osgVerse::IRArg::OBJECT &&
                has(args[1].objType, "Texture"))
            {
                int texture = resolveIndex(args[1]);
                if (texture >= 0) _stateTexture[index] = texture;
                handled = true;
            }
        }
    }
    else if (name == "setAttribute" && !args.empty())
    {
        if (_stateSets.count(index) && args[0].kind == osgVerse::IRArg::OBJECT &&
            args[0].objType == "osg::Material")
        {
            int material = resolveIndex(args[0]);
            if (material >= 0) _stateMaterial[index] = material;
            handled = true;
        }
    }
    else if (name == "setMode" || name == "setTextureMode")
    {
        handled = _stateSets.count(index) != 0;
    }
    else if (name == "Image")
    {
        if (_textures.count(index))
        {
            int image = resolveIndex(args.empty() ? osgVerse::IRArg() : args[0]);
            if (image >= 0) _textureImage[index] = image;
            handled = true;
        }
    }
    else if (name == "setWrap" || name == "setFilter" || name == "WrapS" || name == "WrapT" ||
             name == "WrapR" || name == "FilterMin" || name == "FilterMag")
    {
        handled = _textures.count(index) != 0;
    }
    else if (name == "setAmbient" || name == "setDiffuse" || name == "setSpecular" ||
             name == "setEmission" || name == "setShininess")
    {
        if (_materials.find(index) != _materials.end())
        {
            handleMaterialProp(args, index, name, handled);
        }
    }
    else if (name == "setFileName" || name == "setRange" || name == "setCenter" ||
             name == "setRadius" || name == "setDatabasePath")
    {
        if (_lods.find(index) != _lods.end()) handleLodProp(args, index, name, handled);
    }

    if (!handled) ++_unhandled[name];
}

void GlSceneBuilder::handleGeometryArray(const Args& args, int index,
                                         const std::string& name, bool& handled)
{
    GeomState& geometry = _geoms[index];
    size_t ai = 0;

    if (name == "setTexCoordArray" || name == "setVertexAttribArray")
    {
        if (args.size() < 2) { handled = true; return; }
        if (name == "setVertexAttribArray") { handled = true; return; }
        if (uArg(args, 0) != 0) { handled = true; return; }
        ai = 1;
    }
    if (args.size() <= ai) { handled = true; return; }

    const int array = resolveIndex(args[ai]);
    if (array < 0) { handled = true; return; }

    if (name == "VertexArray" || name == "setVertexArray") geometry.vertex = array;
    else if (name == "NormalArray" || name == "setNormalArray") geometry.normal = array;
    else if (name == "ColorArray" || name == "setColorArray") geometry.color = array;
    else if (name == "setTexCoordArray") geometry.texCoord = array;
    handled = true;
}

void GlSceneBuilder::handlePrimitiveProp(const Args& args, int index,
                                         const std::string& name, bool& handled)
{
    PrimData& prim = _prims[index];
    if (name == "Mode") prim.mode = iArg(args, 0);
    else if (name == "First") prim.first = uArg(args, 0);
    else if (name == "Count") prim.count = uArg(args, 0);
    handled = true;
}

void GlSceneBuilder::handleMaterialProp(const Args& args, int index,
                                        const std::string& name, bool& handled)
{
    MaterialData& material = _materials[index];
    if (name == "setShininess")
    {
        material.shininess = (float)dArg(args, args.size() - 1);
        handled = true;
        return;
    }
    if (args.empty()) return;

    const osgVerse::IRArg& color = args[args.size() - 1];
    if (color.kind != osgVerse::IRArg::VEC4 && color.n < 4) return;

    float* target = material.diffuse;
    if (name == "setAmbient") target = material.ambient;
    else if (name == "setSpecular") target = material.specular;
    else if (name == "setEmission") target = material.emission;

    target[0] = (float)color.v[0];
    target[1] = (float)color.v[1];
    target[2] = (float)color.v[2];
    target[3] = (float)color.v[3];
    handled = true;
}

void GlSceneBuilder::handleLodProp(const Args& args, int index,
                                   const std::string& name, bool& handled)
{
    LodState& lod = _lods[index];
    if (name == "setFileName" && args.size() >= 2)
    {
        lod.files[uArg(args, 0)] = args[1].s;
        handled = true;
    }
    else if (name == "setRange" && args.size() >= 3)
    {
        lod.ranges[uArg(args, 0)] = std::make_pair(dArg(args, 1), dArg(args, 2));
        handled = true;
    }
    else if (name == "setCenter" && !args.empty() && args[0].n >= 3)
    {
        lod.node->center[0] = args[0].v[0];
        lod.node->center[1] = args[0].v[1];
        lod.node->center[2] = args[0].v[2];
        lod.node->hasCenter = true;
        handled = true;
    }
    else if (name == "setRadius")
    {
        lod.node->radius = dArg(args, 0);
        handled = true;
    }
    else if (name == "setDatabasePath")
    {
        handled = true;
    }
}

void GlSceneBuilder::finish()
{
    for (std::map<int, GeomState>::iterator it = _geoms.begin(); it != _geoms.end(); ++it)
        buildGeometry(it->first, it->second);

    for (std::map<int, LodState>::iterator it = _lods.begin(); it != _lods.end(); ++it)
        buildLod(it->second);
}

void GlSceneBuilder::buildLod(LodState& lod)
{
    std::shared_ptr<Node> node = lod.node;
    if (!node) return;

    node->children = lod.children;
    node->files.clear();
    for (std::map<unsigned int, std::string>::const_iterator it = lod.files.begin();
         it != lod.files.end(); ++it)
        if (!it->second.empty()) node->files.push_back(it->second);

    node->childRanges.clear();
    for (size_t i = 0; i < lod.children.size(); ++i)
    {
        std::map<unsigned int, std::pair<double, double> >::const_iterator r =
            lod.ranges.find((unsigned int)i);
        node->childRanges.push_back(r != lod.ranges.end()
                                        ? r->second
                                        : std::make_pair(0.0, kInfinity));
    }
}

void GlSceneBuilder::buildGeometry(int index, GeomState& state)
{
    (void)index;
    std::map<int, ArrayData>::const_iterator vit = _arrays.find(state.vertex);
    if (vit == _arrays.end() || vit->second.components < 3 || vit->second.count() == 0)
        return;

    const ArrayData& positions = vit->second;
    const size_t count = positions.count();

    const ArrayData* normals = NULL;
    const ArrayData* colors = NULL;
    const ArrayData* texCoords = NULL;
    if (state.normal >= 0 && _arrays.count(state.normal)) normals = &_arrays[state.normal];
    if (state.color >= 0 && _arrays.count(state.color)) colors = &_arrays[state.color];
    if (state.texCoord >= 0 && _arrays.count(state.texCoord)) texCoords = &_arrays[state.texCoord];

    std::vector<Vertex> vertices(count);
    Bounds localBounds;
    for (size_t i = 0; i < count; ++i)
    {
        Vertex& v = vertices[i];
        v.position[0] = positions.values[i * positions.components + 0];
        v.position[1] = positions.values[i * positions.components + 1];
        v.position[2] = positions.values[i * positions.components + 2];
        localBounds.expand(v.position);

        v.normal[0] = v.normal[1] = v.normal[2] = 0.0f;
        if (normals && normals->components >= 3 && i < normals->count())
        {
            v.normal[0] = normals->values[i * normals->components + 0];
            v.normal[1] = normals->values[i * normals->components + 1];
            v.normal[2] = normals->values[i * normals->components + 2];
        }

        v.color[0] = v.color[1] = v.color[2] = v.color[3] = 1.0f;
        if (colors && colors->components >= 3 && i < colors->count())
        {
            const float scale = colors->normalized ? (1.0f / 255.0f) : 1.0f;
            v.color[0] = colors->values[i * colors->components + 0] * scale;
            v.color[1] = colors->values[i * colors->components + 1] * scale;
            v.color[2] = colors->values[i * colors->components + 2] * scale;
            if (colors->components >= 4)
                v.color[3] = colors->values[i * colors->components + 3] * scale;
        }

        v.texCoord[0] = v.texCoord[1] = 0.0f;
        if (texCoords && texCoords->components >= 2 && i < texCoords->count())
        {
            v.texCoord[0] = texCoords->values[i * texCoords->components + 0];
            v.texCoord[1] = texCoords->values[i * texCoords->components + 1];
        }
    }

    std::map<GLenum, std::vector<GLuint> > groups;
    if (state.primitives.empty())
    {
        std::vector<GLuint>& list = groups[GL_TRIANGLES];
        list.resize(count);
        for (size_t i = 0; i < count; ++i) list[i] = (GLuint)i;
    }
    else
    {
        for (size_t i = 0; i < state.primitives.size(); ++i)
        {
            std::map<int, PrimData>::const_iterator it = _prims.find(state.primitives[i]);
            if (it != _prims.end()) appendPrimitive(it->second, groups);
        }
    }

    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
    mesh->hasNormals = normals != NULL;
    mesh->useVertexColors = colors != NULL;
    mesh->localBounds = localBounds;
    mesh->diffuse[0] = mesh->diffuse[1] = mesh->diffuse[2] = 0.8f;
    mesh->diffuse[3] = 1.0f;

    if (state.stateSet >= 0)
    {
        std::map<int, int>::const_iterator sit = _stateMaterial.find(state.stateSet);
        if (sit != _stateMaterial.end())
        {
            std::map<int, MaterialData>::const_iterator mit = _materials.find(sit->second);
            if (mit != _materials.end())
                std::memcpy(mesh->diffuse, mit->second.diffuse, sizeof(mesh->diffuse));
        }

        std::map<int, int>::const_iterator tit = _stateTexture.find(state.stateSet);
        if (tit != _stateTexture.end())
        {
            std::map<int, int>::const_iterator iit = _textureImage.find(tit->second);
            if (iit != _textureImage.end())
            {
                std::map<int, GLuint>::const_iterator git = _imageTexture.find(iit->second);
                if (git != _imageTexture.end() && git->second)
                {
                    mesh->texture = git->second;
                    mesh->hasTexture = true;
                }
            }
        }
    }

    glGenBuffers(1, &mesh->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(Vertex)),
                 &vertices[0], GL_STATIC_DRAW);

    for (std::map<GLenum, std::vector<GLuint> >::const_iterator it = groups.begin();
         it != groups.end(); ++it)
    {
        if (it->second.empty()) continue;

        SubMesh sub;
        sub.mode = it->first;
        sub.count = (GLsizei)it->second.size();

        glGenVertexArrays(1, &sub.vao);
        glBindVertexArray(sub.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                              (const void*)offsetof(Vertex, texCoord));

        glGenBuffers(1, &sub.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(it->second.size() * sizeof(GLuint)),
                     &it->second[0], GL_STATIC_DRAW);
        glBindVertexArray(0);

        mesh->subMeshes.push_back(sub);
        ++_draws;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    state.node->mesh = mesh;
}

void GlSceneBuilder::appendPrimitive(const PrimData& prim,
                                     std::map<GLenum, std::vector<GLuint> >& groups)
{
    if (!prim.lengths.empty())
    {
        unsigned int offset = prim.first;
        for (size_t i = 0; i < prim.lengths.size(); ++i)
        {
            appendRun(prim.mode, offset, prim.lengths[i], groups);
            offset += prim.lengths[i];
        }
    }
    else if (!prim.indices.empty())
    {
        expand(prim.mode, prim.indices, groups);
    }
    else
    {
        appendRun(prim.mode, prim.first, prim.count, groups);
    }
}

void GlSceneBuilder::appendRun(int mode, unsigned int first, unsigned int count,
                               std::map<GLenum, std::vector<GLuint> >& groups)
{
    std::vector<unsigned int> src(count);
    for (unsigned int i = 0; i < count; ++i) src[i] = first + i;
    expand(mode, src, groups);
}

void GlSceneBuilder::expand(int mode, const std::vector<unsigned int>& src,
                            std::map<GLenum, std::vector<GLuint> >& groups)
{
    std::vector<GLuint>& out = groups[listMode(mode)];
    switch (mode)
    {
    case 0x0005:   // GL_TRIANGLE_STRIP
        for (size_t i = 2; i < src.size(); ++i)
            addTriangle(out, src[i - 2], src[i - 1], src[i], (i & 1) != 0);
        break;
    case 0x0006:   // GL_TRIANGLE_FAN
    case 0x0009:   // GL_POLYGON
        for (size_t i = 2; i < src.size(); ++i)
            addTriangle(out, src[0], src[i - 1], src[i], false);
        break;
    case 0x0007:   // GL_QUADS
        for (size_t i = 0; i + 4 <= src.size(); i += 4)
        {
            addTriangle(out, src[i + 0], src[i + 1], src[i + 2], false);
            addTriangle(out, src[i + 0], src[i + 2], src[i + 3], false);
        }
        break;
    case 0x0008:   // GL_QUAD_STRIP
        for (size_t i = 0; i + 4 <= src.size(); i += 2)
        {
            addTriangle(out, src[i + 0], src[i + 1], src[i + 2], false);
            addTriangle(out, src[i + 1], src[i + 3], src[i + 2], false);
        }
        break;
    case 0x0002:   // GL_LINE_LOOP
        for (size_t i = 0; i < src.size(); ++i)
        {
            out.push_back(src[i]);
            out.push_back(src[(i + 1) % src.size()]);
        }
        break;
    case 0x0003:   // GL_LINE_STRIP
        for (size_t i = 0; i + 1 < src.size(); ++i)
        {
            out.push_back(src[i]);
            out.push_back(src[i + 1]);
        }
        break;
    default:
        out.insert(out.end(), src.begin(), src.end());
        break;
    }
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------
void GlSceneBuilder::uploadTexture(int index, const osgVerse::IRObject& obj)
{
    ImageRGBA image;
    bool decoded = false;

    if (obj.imageDecision == osgVerse::IMAGE_INLINE_FILE && !obj.pixelData.empty())
    {
        decoded = decodeImageMemory(&obj.pixelData[0], obj.pixelData.size(), obj.fileName, image);
    }
    else if (obj.imageDecision == osgVerse::IMAGE_INLINE_DATA && !obj.pixelData.empty())
    {
        const int width = obj.width, height = obj.height;
        const size_t components = (obj.pixelFormat == GL_RGBA) ? 4 : 3;
        if (obj.dataType == GL_UNSIGNED_BYTE && width > 0 && height > 0 &&
            (obj.pixelFormat == GL_RGB || obj.pixelFormat == GL_RGBA) &&
            obj.pixelData.size() >= (size_t)width * height * components)
        {
            image.width = width;
            image.height = height;
            image.pixels.resize((size_t)width * height * 4);
            const unsigned char* src = &obj.pixelData[0];
            for (size_t i = 0, n = (size_t)width * height; i < n; ++i)
            {
                image.pixels[i * 4 + 0] = src[i * components + 0];
                image.pixels[i * 4 + 1] = src[i * components + 1];
                image.pixels[i * 4 + 2] = src[i * components + 2];
                image.pixels[i * 4 + 3] = (components == 4) ? src[i * components + 3] : 255;
            }
            decoded = true;
        }
    }
    else if (!obj.fileName.empty())
    {
        decoded = decodeImageFile(resolvePath(obj.fileName), image);
    }

    if (!decoded || !image.valid())
    {
        ++_failedImages;
        _imageTexture[index] = 0;
        return;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width, image.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, &image.pixels[0]);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    _imageTexture[index] = texture;
    ++_textureCount;
}

// ---------------------------------------------------------------------------
// Paging and bounds
// ---------------------------------------------------------------------------
void GlSceneBuilder::loadPagedLevels(int levels)
{
    if (levels <= 0) return;

    std::vector<std::shared_ptr<Node> > paged;
    collectPagedNodes(root(), paged);

    for (size_t i = 0; i < paged.size(); ++i)
    {
        std::shared_ptr<Node> node = paged[i];
        if (node->files.empty() || !node->pagedChildren.empty()) continue;

        const std::string& file = node->files.front();
        const std::string path = resolvePath(file);

        osgVerse::SceneIR ir;
        if (!osgVerse::readSceneIR(path, ir))
        {
            ++_failedChildren;
            continue;
        }

        std::shared_ptr<GlSceneBuilder> child = std::make_shared<GlSceneBuilder>(path);
        osgVerse::buildScene(ir, *child);
        child->finish();
        _childBuilders.push_back(child);
        child->loadPagedLevels(levels - 1);

        node->pagedChildren.push_back(child->root());
        ++_loadedChildren;

        // Report the whole tree, not just this file.
        _created += child->created();
        _draws += child->draws();
        _textureCount += child->textures();
        _failedImages += child->failedImages();
        _failedChildren += child->failedChildren();
        _loadedChildren += child->loadedChildren();
    }
}

void GlSceneBuilder::collectPagedNodes(const std::shared_ptr<Node>& node,
                                       std::vector<std::shared_ptr<Node> >& out) const
{
    if (!node) return;
    if (node->kind == Node::PAGED_LOD) out.push_back(node);
    for (size_t i = 0; i < node->children.size(); ++i)
        collectPagedNodes(node->children[i], out);
}

void GlSceneBuilder::computeBounds()
{
    _sceneBounds = Bounds();
    if (root()) computeNodeBounds(root(), _sceneBounds);
}

void GlSceneBuilder::computeNodeBounds(const std::shared_ptr<Node>& node, Bounds& bounds)
{
    Bounds local;
    if (node->mesh && node->mesh->localBounds.valid()) local.expand(node->mesh->localBounds);

    for (size_t i = 0; i < node->children.size(); ++i)
    {
        Bounds childBounds;
        computeNodeBounds(node->children[i], childBounds);
        local.expand(childBounds.transformed(node->children[i]->matrix));
    }
    for (size_t i = 0; i < node->pagedChildren.size(); ++i)
    {
        Bounds childBounds;
        computeNodeBounds(node->pagedChildren[i], childBounds);
        local.expand(childBounds.transformed(node->pagedChildren[i]->matrix));
    }

    if (local.valid())
    {
        for (int k = 0; k < 3; ++k) node->boundCenter[k] = (local.min[k] + local.max[k]) * 0.5;
        const double dx = local.max[0] - local.min[0];
        const double dy = local.max[1] - local.min[1];
        const double dz = local.max[2] - local.min[2];
        node->boundRadius = 0.5 * std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    else if (node->hasCenter)
    {
        for (int k = 0; k < 3; ++k) node->boundCenter[k] = node->center[k];
        node->boundRadius = node->radius;
    }

    bounds.expand(local);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
std::shared_ptr<Node> GlSceneBuilder::root() const
{
    std::map<int, std::shared_ptr<Node> >::const_iterator it = _nodes.find(_rootIndex);
    return it != _nodes.end() ? it->second : std::shared_ptr<Node>();
}

std::shared_ptr<Node> GlSceneBuilder::nodeFor(const osgVerse::IRArg& arg) const
{
    const int index = resolveIndex(arg);
    if (index < 0) return std::shared_ptr<Node>();
    std::map<int, std::shared_ptr<Node> >::const_iterator it = _nodes.find(index);
    return it != _nodes.end() ? it->second : std::shared_ptr<Node>();
}

std::shared_ptr<Node> GlSceneBuilder::nodeFor(const Args& args, size_t i) const
{
    static const osgVerse::IRArg empty;
    return nodeFor(i < args.size() ? args[i] : empty);
}

int GlSceneBuilder::resolveIndex(const osgVerse::IRArg& arg) const
{
    if (arg.kind != osgVerse::IRArg::OBJECT || arg.objId.empty()) return -1;
    std::map<std::string, int>::const_iterator it =
        _byKey.find(nsFor(arg.objType) + "#" + arg.objId);
    return it != _byKey.end() ? it->second : -1;
}

std::string GlSceneBuilder::resolvePath(const std::string& name) const
{
    std::string path = name;
    for (size_t i = 0; i < path.size(); ++i)
        if (path[i] == '\\') path[i] = '/';

    if (path.empty()) return path;
    if (path[0] == '/' || (path.size() > 1 && path[1] == ':')) return path;
    return _baseDir + "/" + path;
}

unsigned int GlSceneBuilder::uArg(const Args& args, size_t i)
{
    static const osgVerse::IRArg empty;
    const osgVerse::IRArg& value = i < args.size() ? args[i] : empty;
    return (unsigned int)(value.kind == osgVerse::IRArg::INT ? value.i : value.u);
}

int GlSceneBuilder::iArg(const Args& args, size_t i)
{
    static const osgVerse::IRArg empty;
    const osgVerse::IRArg& value = i < args.size() ? args[i] : empty;
    return (int)(value.kind == osgVerse::IRArg::INT ? value.i : (long long)value.u);
}

double GlSceneBuilder::dArg(const Args& args, size_t i)
{
    static const osgVerse::IRArg empty;
    const osgVerse::IRArg& value = i < args.size() ? args[i] : empty;
    return value.kind == osgVerse::IRArg::DOUBLE ? value.d : (double)value.i;
}

}  // namespace gl_ir
