// opengl_integration: read an .osgt/.osgb file and render it with plain
// OpenGL 3.3 core (no osg, no VSG).
//
// The reader emits SceneIR; gl_scene_builder.h turns it into VAOs / textures.
// FreeGLUT provides the window and input, GLEW the GL 3.3 entry points, and
// stb_image (compiled into image_decode.cpp) decodes the embedded JPEG/PNG
// textures -- no libjpeg/libpng dependency.
//
// Build: -DENABLE_OPENGL=ON
// Usage: opengl_integration <file.osgt|file.osgb> [--load-levels N]
//                            [--screenshot out.ppm] [--list]

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "SceneIR.h"
#include "gl_scene_builder.h"

#ifdef OSGVERSE_GENERIC_USE_STATIC_ACTIVATION
#include "GenericStaticWrappers.h"
#endif

namespace
{

struct Options
{
    std::string filename;
    std::string screenshot;
    int loadLevels = 1;
    int width = 1280;
    int height = 720;
    bool listOnly = false;
};

struct Camera
{
    double target[3] = {0.0, 0.0, 0.0};
    double distance = 3.0;
    double yaw = -1.0;
    double pitch = 0.45;
};

struct Uniforms
{
    GLint mvp = -1;
    GLint model = -1;
    GLint diffuse = -1;
    GLint hasTexture = -1;
    GLint hasNormals = -1;
    GLint useVertexColors = -1;
    GLint texture = -1;
};

Options g_options;
std::shared_ptr<gl_ir::GlSceneBuilder> g_builder;
std::shared_ptr<gl_ir::Node> g_root;
Camera g_camera;
gl_ir::Mat4 g_viewProjection;
double g_sceneRadius = 1.0;
double g_eye[3] = {0.0, 0.0, 0.0};

GLuint g_program = 0;
Uniforms g_uniforms;
int g_width = 1280;
int g_height = 720;
bool g_dragging = false;
int g_lastX = 0;
int g_lastY = 0;

// ---------------------------------------------------------------------------
// Math
// ---------------------------------------------------------------------------
gl_ir::Mat4 makeViewProjection(const Camera& camera)
{
    const double cp = std::cos(camera.pitch);
    const double eye[3] = {
        camera.target[0] + camera.distance * cp * std::cos(camera.yaw),
        camera.target[1] + camera.distance * cp * std::sin(camera.yaw),
        camera.target[2] + camera.distance * std::sin(camera.pitch)
    };
    g_eye[0] = eye[0];
    g_eye[1] = eye[1];
    g_eye[2] = eye[2];

    double forward[3] = {
        camera.target[0] - eye[0], camera.target[1] - eye[1], camera.target[2] - eye[2]
    };
    double length = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] +
                              forward[2] * forward[2]);
    if (length < 1e-9) length = 1.0;
    for (int i = 0; i < 3; ++i) forward[i] /= length;

    // Z-up world: right = forward x up, up' = right x forward.
    const double worldUp[3] = {0.0, 0.0, 1.0};
    double right[3] = {
        forward[1] * worldUp[2] - forward[2] * worldUp[1],
        forward[2] * worldUp[0] - forward[0] * worldUp[2],
        forward[0] * worldUp[1] - forward[1] * worldUp[0]
    };
    length = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
    if (length < 1e-9) { right[0] = 1.0; right[1] = 0.0; right[2] = 0.0; length = 1.0; }
    for (int i = 0; i < 3; ++i) right[i] /= length;

    double up[3] = {
        right[1] * forward[2] - right[2] * forward[1],
        right[2] * forward[0] - right[0] * forward[2],
        right[0] * forward[1] - right[1] * forward[0]
    };

    const double dotRight = right[0] * eye[0] + right[1] * eye[1] + right[2] * eye[2];
    const double dotUp = up[0] * eye[0] + up[1] * eye[1] + up[2] * eye[2];
    const double dotForward = forward[0] * eye[0] + forward[1] * eye[1] + forward[2] * eye[2];

    gl_ir::Mat4 view;
    view.m[0] = (float)right[0];   view.m[4] = (float)right[1];
    view.m[8] = (float)right[2];   view.m[12] = (float)-dotRight;
    view.m[1] = (float)up[0];      view.m[5] = (float)up[1];
    view.m[9] = (float)up[2];      view.m[13] = (float)-dotUp;
    view.m[2] = (float)-forward[0]; view.m[6] = (float)-forward[1];
    view.m[10] = (float)-forward[2]; view.m[14] = (float)dotForward;

    const double nearPlane = std::max(g_sceneRadius * 0.001, 0.01);
    const double farPlane = std::max(g_sceneRadius * 20.0, camera.distance * 4.0);
    const double aspect = (double)g_width / (double)std::max(g_height, 1);
    const double f = 1.0 / std::tan(0.5 * 45.0 * 3.14159265358979323846 / 180.0);

    gl_ir::Mat4 projection;
    std::memset(projection.m, 0, sizeof(projection.m));
    projection.m[0] = (float)(f / aspect);
    projection.m[5] = (float)f;
    projection.m[10] = (float)((farPlane + nearPlane) / (nearPlane - farPlane));
    projection.m[11] = -1.0f;
    projection.m[14] = (float)(2.0 * farPlane * nearPlane / (nearPlane - farPlane));

    return projection * view;
}

// ---------------------------------------------------------------------------
// Shaders
// ---------------------------------------------------------------------------
const char* kVertexSource = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aTexCoord;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 vNormal;
out vec4 vColor;
out vec2 vTexCoord;

void main()
{
    gl_Position = uMVP * vec4(aPosition, 1.0);
    vNormal = mat3(uModel) * aNormal;
    vColor = aColor;
    vTexCoord = aTexCoord;
}
)";

const char* kFragmentSource = R"(
#version 330 core
in vec3 vNormal;
in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform bool uHasNormals;
uniform bool uUseVertexColors;
uniform vec4 uDiffuse;

out vec4 fragColor;

void main()
{
    vec4 base = uHasTexture ? texture(uTexture, vTexCoord) : uDiffuse;
    if (uUseVertexColors) base *= vColor;

    vec3 color = base.rgb;
    if (uHasNormals)
    {
        // Two-sided, ambient-heavy shading: photogrammetry textures already
        // contain their baked lighting, the light only adds some shape.
        vec3 n = normalize(vNormal);
        vec3 l = normalize(vec3(0.35, -0.55, 0.75));
        float ndl = abs(dot(n, l));
        color *= 0.78 + 0.35 * ndl;
    }
    fragColor = vec4(color, base.a);
}
)";

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length + 1);
        glGetShaderInfoLog(shader, length, NULL, &log[0]);
        std::cerr << "Shader compile failed: " << &log[0] << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool initGL()
{
    GLuint vertex = compileShader(GL_VERTEX_SHADER, kVertexSource);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, kFragmentSource);
    if (!vertex || !fragment) return false;

    g_program = glCreateProgram();
    glAttachShader(g_program, vertex);
    glAttachShader(g_program, fragment);
    glLinkProgram(g_program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(g_program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint length = 0;
        glGetProgramiv(g_program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length + 1);
        glGetProgramInfoLog(g_program, length, NULL, &log[0]);
        std::cerr << "Program link failed: " << &log[0] << std::endl;
        return false;
    }

    g_uniforms.mvp = glGetUniformLocation(g_program, "uMVP");
    g_uniforms.model = glGetUniformLocation(g_program, "uModel");
    g_uniforms.diffuse = glGetUniformLocation(g_program, "uDiffuse");
    g_uniforms.hasTexture = glGetUniformLocation(g_program, "uHasTexture");
    g_uniforms.hasNormals = glGetUniformLocation(g_program, "uHasNormals");
    g_uniforms.useVertexColors = glGetUniformLocation(g_program, "uUseVertexColors");
    g_uniforms.texture = glGetUniformLocation(g_program, "uTexture");

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);   // photogrammetry winding is not always consistent
    glClearColor(0.05f, 0.07f, 0.09f, 1.0f);
    return true;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------
void drawMesh(const gl_ir::Mesh& mesh, const gl_ir::Mat4& model)
{
    const gl_ir::Mat4 mvp = g_viewProjection * model;
    glUniformMatrix4fv(g_uniforms.mvp, 1, GL_FALSE, mvp.m);
    glUniformMatrix4fv(g_uniforms.model, 1, GL_FALSE, model.m);
    glUniform4fv(g_uniforms.diffuse, 1, mesh.diffuse);
    glUniform1i(g_uniforms.hasTexture, mesh.hasTexture ? 1 : 0);
    glUniform1i(g_uniforms.hasNormals, mesh.hasNormals ? 1 : 0);
    glUniform1i(g_uniforms.useVertexColors, mesh.useVertexColors ? 1 : 0);
    if (mesh.hasTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.texture);
        glUniform1i(g_uniforms.texture, 0);
    }

    for (size_t i = 0; i < mesh.subMeshes.size(); ++i)
    {
        const gl_ir::SubMesh& sub = mesh.subMeshes[i];
        glBindVertexArray(sub.vao);
        glDrawElements(sub.mode, sub.count, GL_UNSIGNED_INT, NULL);
    }
    glBindVertexArray(0);
}

void drawNode(const std::shared_ptr<gl_ir::Node>& node, const gl_ir::Mat4& parent)
{
    if (!node) return;
    const gl_ir::Mat4 model = parent * node->matrix;

    switch (node->kind)
    {
    case gl_ir::Node::GEOMETRY:
        if (node->mesh) drawMesh(*node->mesh, model);
        break;

    case gl_ir::Node::LOD:
    {
        const double* center = node->hasCenter ? node->center : node->boundCenter;
        double worldCenter[3];
        model.transformPoint(center, worldCenter);
        const double dx = g_eye[0] - worldCenter[0];
        const double dy = g_eye[1] - worldCenter[1];
        const double dz = g_eye[2] - worldCenter[2];
        const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        const double radius = node->radius > 0.0 ? node->radius : node->boundRadius;

        int chosen = -1;
        for (size_t i = 0; i < node->children.size(); ++i)
        {
            double minRange = 0.0, maxRange = 1.0e30;
            if (i < node->childRanges.size())
            {
                minRange = node->childRanges[i].first;
                maxRange = node->childRanges[i].second;
            }
            if (distance <= radius * maxRange && distance >= radius * minRange)
            {
                chosen = (int)i;
                break;
            }
        }
        if (chosen < 0 && !node->children.empty()) chosen = (int)node->children.size() - 1;
        if (chosen >= 0) drawNode(node->children[chosen], model);
        break;
    }

    case gl_ir::Node::PAGED_LOD:
        // With a loaded external child, osg hides the in-memory stand-in.
        if (!node->pagedChildren.empty())
        {
            for (size_t i = 0; i < node->pagedChildren.size(); ++i)
                drawNode(node->pagedChildren[i], model);
        }
        else
        {
            for (size_t i = 0; i < node->children.size(); ++i)
                drawNode(node->children[i], model);
        }
        break;

    default:
        for (size_t i = 0; i < node->children.size(); ++i)
            drawNode(node->children[i], model);
        break;
    }
}

void renderScene()
{
    glViewport(0, 0, g_width, g_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!g_root) return;

    g_viewProjection = makeViewProjection(g_camera);
    glUseProgram(g_program);
    drawNode(g_root, gl_ir::Mat4());
    glUseProgram(0);
}

void display()
{
    renderScene();
    glutSwapBuffers();
}

// ---------------------------------------------------------------------------
// Camera / input
// ---------------------------------------------------------------------------
void clampCamera()
{
    g_camera.distance = std::max(g_sceneRadius * 0.02, std::min(g_camera.distance, g_sceneRadius * 50.0));
    g_camera.pitch = std::max(-1.45, std::min(g_camera.pitch, 1.45));
}

void resetCamera()
{
    gl_ir::Bounds bounds;
    if (g_builder) bounds = g_builder->sceneBounds();
    if (bounds.valid())
    {
        for (int i = 0; i < 3; ++i)
            g_camera.target[i] = (bounds.min[i] + bounds.max[i]) * 0.5;
        const double dx = bounds.max[0] - bounds.min[0];
        const double dy = bounds.max[1] - bounds.min[1];
        const double dz = bounds.max[2] - bounds.min[2];
        g_sceneRadius = std::max(0.5 * std::sqrt(dx * dx + dy * dy + dz * dz), 1e-3);
    }
    else
    {
        g_camera.target[0] = g_camera.target[1] = g_camera.target[2] = 0.0;
        g_sceneRadius = 1.0;
    }
    g_camera.distance = g_sceneRadius * 2.5;
    g_camera.yaw = -1.0;
    g_camera.pitch = 0.45;
    clampCamera();
}

void reshape(int width, int height)
{
    g_width = std::max(width, 1);
    g_height = std::max(height, 1);
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    if (button == 3 && state == GLUT_DOWN)         // wheel up
    {
        g_camera.distance *= 0.9;
        clampCamera();
        glutPostRedisplay();
    }
    else if (button == 4 && state == GLUT_DOWN)    // wheel down
    {
        g_camera.distance *= 1.1;
        clampCamera();
        glutPostRedisplay();
    }
    else if (button == GLUT_LEFT_BUTTON)
    {
        g_dragging = (state == GLUT_DOWN);
        g_lastX = x;
        g_lastY = y;
    }
}

void motion(int x, int y)
{
    if (!g_dragging) return;
    g_camera.yaw -= (x - g_lastX) * 0.006;
    g_camera.pitch += (y - g_lastY) * 0.006;
    g_lastX = x;
    g_lastY = y;
    clampCamera();
    glutPostRedisplay();
}

void keyboard(unsigned char key, int, int)
{
    if (key == 27) exit(0);                          // ESC
    else if (key == 'r' || key == 'R') resetCamera();
    else if (key == '+' || key == '=') { g_camera.distance *= 0.9; clampCamera(); }
    else if (key == '-' || key == '_') { g_camera.distance *= 1.1; clampCamera(); }
    else return;
    glutPostRedisplay();
}

// ---------------------------------------------------------------------------
// Screenshot (PPM, so no image writer dependency)
// ---------------------------------------------------------------------------
void writePPM(const std::string& filename)
{
    std::vector<unsigned char> pixels((size_t)g_width * g_height * 3);
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, g_width, g_height, GL_RGB, GL_UNSIGNED_BYTE, &pixels[0]);

    FILE* fp = std::fopen(filename.c_str(), "wb");
    if (!fp)
    {
        std::cerr << "Cannot write " << filename << std::endl;
        return;
    }
    std::fprintf(fp, "P6\n%d %d\n255\n", g_width, g_height);
    // glReadPixels is bottom-up; PPM wants top-down.
    for (int y = g_height - 1; y >= 0; --y)
        std::fwrite(&pixels[(size_t)y * g_width * 3], 1, (size_t)g_width * 3, fp);
    std::fclose(fp);
    std::cout << "Screenshot: " << filename << std::endl;
}

void screenshotTimer(int)
{
    renderScene();
    writePPM(g_options.screenshot);
    std::exit(0);
}

// ---------------------------------------------------------------------------
// Scene loading
// ---------------------------------------------------------------------------
bool loadScene()
{
    osgVerse::SceneIR ir;
    if (!osgVerse::readSceneIR(g_options.filename, ir))
    {
        std::cerr << "Failed to read " << g_options.filename << std::endl;
        return false;
    }

    std::cout << "IR objects : " << ir.objects.size()
              << " (" << ir.events.size() << " events)" << std::endl;

    g_builder = std::make_shared<gl_ir::GlSceneBuilder>(g_options.filename);
    osgVerse::buildScene(ir, *g_builder);
    g_builder->loadPagedLevels(g_options.loadLevels);
    g_builder->computeBounds();

    g_root = g_builder->root();
    if (!g_root)
    {
        std::cerr << "No renderable root rebuilt." << std::endl;
        return false;
    }

    std::cout << "Nodes      : " << g_builder->created() << std::endl;
    std::cout << "Draws      : " << g_builder->draws() << std::endl;
    std::cout << "Textures   : " << g_builder->textures() << std::endl;
    if (g_builder->pagedLods())
        std::cout << "PagedLODs  : " << g_builder->pagedLods() << std::endl;
    if (g_builder->loadedChildren())
        std::cout << "Paged files: " << g_builder->loadedChildren() << std::endl;
    if (g_builder->failedImages())
        std::cout << "Missing    : " << g_builder->failedImages() << " images" << std::endl;
    if (g_builder->failedChildren())
        std::cout << "Missing    : " << g_builder->failedChildren() << " paged children" << std::endl;

    const gl_ir::Bounds& bounds = g_builder->sceneBounds();
    if (bounds.valid())
        std::cout << "Bounds     : " << bounds.min[0] << ", " << bounds.min[1] << ", "
                  << bounds.min[2] << " - " << bounds.max[0] << ", " << bounds.max[1]
                  << ", " << bounds.max[2] << std::endl;

    resetCamera();
    return true;
}

bool listScene()
{
    osgVerse::SceneIR ir;
    if (!osgVerse::readSceneIR(g_options.filename, ir)) return false;

    std::map<std::string, unsigned int> classes;
    int pagedLods = 0;
    for (size_t i = 0; i < ir.objects.size(); ++i)
    {
        ++classes[ir.objects[i].className];
        if (ir.objects[i].className.find("PagedLOD") != std::string::npos) ++pagedLods;
    }

    std::cout << "IR objects : " << ir.objects.size()
              << " (" << ir.events.size() << " events)" << std::endl;
    for (std::map<std::string, unsigned int>::const_iterator it = classes.begin();
         it != classes.end(); ++it)
        std::cout << "  " << it->second << "  " << it->first << std::endl;
    if (pagedLods) std::cout << "PagedLODs  : " << pagedLods << std::endl;
    return true;
}

bool parseOptions(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: opengl_integration <file.osgt|file.osgb> [--load-levels N]\n"
                     "                           [--screenshot out.ppm] [--list]\n";
        return false;
    }

    g_options.filename = argv[1];
    for (int i = 2; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--list") g_options.listOnly = true;
        else if (arg == "--load-levels" && i + 1 < argc)
            g_options.loadLevels = std::atoi(argv[++i]);
        else if (arg == "--screenshot" && i + 1 < argc)
            g_options.screenshot = argv[++i];
        else if (arg == "--width" && i + 1 < argc)
            g_options.width = std::atoi(argv[++i]);
        else if (arg == "--height" && i + 1 < argc)
            g_options.height = std::atoi(argv[++i]);
        else
            std::cerr << "Ignoring unknown option " << arg << std::endl;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    if (!parseOptions(argc, argv)) return 1;
    if (g_options.listOnly) return listScene() ? 0 : 1;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitWindowSize(g_options.width, g_options.height);
    glutCreateWindow("generic_wrappers - OpenGL core");

    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK)
    {
        std::cerr << "glewInit failed: " << glewGetErrorString(glewStatus) << std::endl;
        return 2;
    }
    while (glGetError() != GL_NO_ERROR) {}   // glewInit may leave GL_INVALID_ENUM

    std::cout << "OpenGL     : " << glGetString(GL_VERSION) << " / "
              << glGetString(GL_RENDERER) << std::endl;

    if (!initGL()) return 3;
    if (!loadScene()) return 4;

    g_width = g_options.width;
    g_height = g_options.height;

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    if (!g_options.screenshot.empty())
        glutTimerFunc(600, screenshotTimer, 0);

    glutMainLoop();
    return 0;
}
