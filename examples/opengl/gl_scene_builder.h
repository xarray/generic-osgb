#ifndef OSGVERSE_GENERIC_WRAPPERS_GL_SCENE_BUILDER_H
#define OSGVERSE_GENERIC_WRAPPERS_GL_SCENE_BUILDER_H

// gl_scene_builder: replay SceneIR into a small OpenGL 3.3 core scene graph.
//
// It mirrors examples/vsg/vsg_scene_builder.h: the reader emits a flat stream of
// object creations and commands, and this adapter turns the osg-specific
// commands (addChild / setVertexArray / setTextureAttribute / setRange ...) into
// VAOs, textures and a lightweight node tree. Nothing from osg is linked.

#include <GL/glew.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "SceneIR.h"

namespace gl_ir
{

// Column-major 4x4 matrix, matching osg's Matrixd layout and GLSL's mat4.
struct Mat4
{
    float m[16];

    Mat4();
    static Mat4 fromDoubles(const double* values);
    Mat4 operator*(const Mat4& other) const;
    void transformPoint(const double in[3], double out[3]) const;
};

struct Bounds
{
    double min[3];
    double max[3];

    Bounds();
    bool valid() const;
    void expand(const float point[3]);
    void expand(const double point[3]);
    void expand(const Bounds& other);
    Bounds transformed(const Mat4& matrix) const;
};

struct SubMesh
{
    GLuint vao = 0;
    GLuint ebo = 0;
    GLsizei count = 0;
    GLenum mode = GL_TRIANGLES;
};

struct Mesh
{
    GLuint vbo = 0;
    std::vector<SubMesh> subMeshes;
    GLuint texture = 0;
    bool hasTexture = false;
    bool hasNormals = false;
    bool useVertexColors = false;
    float diffuse[4];
    Bounds localBounds;
};

struct Node
{
    enum Kind { GROUP, TRANSFORM, GEOMETRY, LOD, PAGED_LOD, SWITCH };

    Kind kind = GROUP;
    Mat4 matrix;

    std::shared_ptr<Mesh> mesh;
    std::vector<std::shared_ptr<Node> > children;

    // LOD / PagedLOD
    std::vector<std::pair<double, double> > childRanges;
    std::vector<std::string> files;
    double center[3];
    bool hasCenter = false;
    double radius = 0.0;
    std::vector<std::shared_ptr<Node> > pagedChildren;

    // Computed bottom-up in computeBounds().
    double boundCenter[3];
    double boundRadius = 0.0;
};

class GlSceneBuilder : public osgVerse::ISceneBuilder
{
public:
    explicit GlSceneBuilder(const std::string& filename);

    void onObject(const osgVerse::IRObject& object, int index) override;
    void onCommand(const osgVerse::IRObject& object, int index,
                   const osgVerse::IRCommand& command) override;
    void finish() override;

    std::shared_ptr<Node> root() const;
    void loadPagedLevels(int levels);
    void computeBounds();

    const Bounds& sceneBounds() const { return _sceneBounds; }
    unsigned int created() const { return _created; }
    unsigned int draws() const { return _draws; }
    unsigned int pagedLods() const { return _pagedLods; }
    unsigned int failedImages() const { return _failedImages; }
    unsigned int failedChildren() const { return _failedChildren; }
    unsigned int loadedChildren() const { return _loadedChildren; }
    unsigned int textures() const { return _textureCount; }
    const std::map<std::string, unsigned int>& unhandled() const { return _unhandled; }

protected:
    typedef std::vector<osgVerse::IRArg> Args;

    enum Kind { K_GENERIC, K_GROUP, K_TRANSFORM, K_LOD, K_PAGEDLOD, K_PROXY, K_SWITCH,
                K_GEOMETRY, K_ARRAY, K_PRIMITIVE, K_IMAGE, K_STATESET, K_TEXTURE, K_MATERIAL };

    struct ArrayData
    {
        int components = 0;
        bool normalized = false;   // integer colour arrays (Vec4ubArray) need /255
        std::vector<float> values;

        size_t count() const { return components > 0 ? values.size() / components : 0; }
    };

    struct PrimData
    {
        int mode = 0;
        unsigned int first = 0;
        unsigned int count = 0;
        std::vector<unsigned int> indices;
        std::vector<unsigned int> lengths;
    };

    struct GeomState
    {
        std::shared_ptr<Node> node;
        int vertex = -1;
        int normal = -1;
        int color = -1;
        int texCoord = -1;
        int stateSet = -1;
        std::vector<int> primitives;
    };

    struct LodState
    {
        bool paged = false;
        std::shared_ptr<Node> node;
        std::vector<std::shared_ptr<Node> > children;
        std::map<unsigned int, std::string> files;
        std::map<unsigned int, std::pair<double, double> > ranges;
    };

    struct MaterialData
    {
        float ambient[4];
        float diffuse[4];
        float specular[4];
        float emission[4];
        float shininess = 0.0f;

        MaterialData();
    };

    struct Vertex
    {
        float position[3];
        float normal[3];
        float color[4];
        float texCoord[2];
    };

    static Kind classify(const std::string& className);
    static std::string nsFor(const std::string& className);

    void addChildTo(std::shared_ptr<Node> parent, std::shared_ptr<Node> child);
    std::shared_ptr<Node> nodeFor(const osgVerse::IRArg& arg) const;
    std::shared_ptr<Node> nodeFor(const Args& args, size_t i) const;
    int resolveIndex(const osgVerse::IRArg& arg) const;

    void handleGeometryArray(const Args& args, int index, const std::string& name, bool& handled);
    void handlePrimitiveProp(const Args& args, int index, const std::string& name, bool& handled);
    void handleStateSetProp(const Args& args, int index, const std::string& name, bool& handled);
    void handleMaterialProp(const Args& args, int index, const std::string& name, bool& handled);
    void handleLodProp(const Args& args, int index, const std::string& name, bool& handled);

    void buildGeometry(int index, GeomState& state);
    void buildLod(LodState& state);
    void uploadTexture(int index, const osgVerse::IRObject& object);
    void collectPagedNodes(const std::shared_ptr<Node>& node,
                           std::vector<std::shared_ptr<Node> >& out) const;
    void computeNodeBounds(const std::shared_ptr<Node>& node, Bounds& bounds);

    std::string resolvePath(const std::string& name) const;
    static void appendPrimitive(const PrimData& prim, std::map<GLenum, std::vector<GLuint> >& groups);
    static void appendRun(int mode, unsigned int first, unsigned int count,
                          std::map<GLenum, std::vector<GLuint> >& groups);
    static void expand(int mode, const std::vector<unsigned int>& src,
                       std::map<GLenum, std::vector<GLuint> >& groups);

    static unsigned int uArg(const Args& args, size_t i);
    static int iArg(const Args& args, size_t i);
    static double dArg(const Args& args, size_t i);

    std::string _filename;
    std::string _baseDir;
    std::map<int, std::shared_ptr<Node> > _nodes;
    std::map<std::string, int> _byKey;
    std::map<int, ArrayData> _arrays;
    std::map<int, PrimData> _prims;
    std::map<int, GeomState> _geoms;
    std::map<int, LodState> _lods;
    std::set<int> _stateSets;
    std::set<int> _textures;
    std::map<int, int> _stateTexture;
    std::map<int, int> _stateMaterial;
    std::map<int, int> _textureImage;
    std::map<int, GLuint> _imageTexture;
    std::map<int, MaterialData> _materials;
    std::vector<std::shared_ptr<GlSceneBuilder> > _childBuilders;

    std::map<std::string, unsigned int> _unhandled;
    Bounds _sceneBounds;

    int _rootIndex = -1;
    unsigned int _created = 0;
    unsigned int _draws = 0;
    unsigned int _pagedLods = 0;
    unsigned int _failedImages = 0;
    unsigned int _failedChildren = 0;
    unsigned int _loadedChildren = 0;
    unsigned int _textureCount = 0;
};

}  // namespace gl_ir

#endif
