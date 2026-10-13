// GenericMath.h - Standalone math & base type layer for the generic_wrappers
// re-implementation of the OpenSceneGraph serialization subsystem.
//
// This header intentionally defines a minimal set of types inside namespace
// `osg` so that the (hand-modified) generic_osg serialization files can be
// compiled WITHOUT linking against the real osg libraries. It only covers what
// the read bodies and serializer macros actually need: reference counting,
// math types, bounding types and GL typedefs.

#ifndef OSGVERSE_GENERIC_WRAPPERS_GENERIC_MATH_H
#define OSGVERSE_GENERIC_WRAPPERS_GENERIC_MATH_H

#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <iostream>

// Version constant used by reserializing_test (was osg/Version in real osg).
#define OPENSCENEGRAPH_SOVERSION 180

// ---------------------------------------------------------------------------
// GL constants (common subset used by the serialization files).
// All guarded so per-file #ifndef fallbacks in generic_osg never conflict.
// ---------------------------------------------------------------------------
#ifndef GL_NONE
#define GL_NONE 0
#endif
#ifndef GL_FALSE
#define GL_FALSE 0
#endif
#ifndef GL_TRUE
#define GL_TRUE 1
#endif
#ifndef GL_ZERO
#define GL_ZERO 0
#endif
#ifndef GL_ONE
#define GL_ONE 1
#endif

#ifndef GL_DEPTH_BUFFER_BIT
#define GL_DEPTH_BUFFER_BIT 0x00000100
#endif
#ifndef GL_STENCIL_BUFFER_BIT
#define GL_STENCIL_BUFFER_BIT 0x00000400
#endif
#ifndef GL_COLOR_BUFFER_BIT
#define GL_COLOR_BUFFER_BIT 0x00004000
#endif
#ifndef GL_ACCUM_BUFFER_BIT
#define GL_ACCUM_BUFFER_BIT 0x00000200
#endif

#ifndef GL_NEVER
#define GL_NEVER 0x0200
#endif
#ifndef GL_LESS
#define GL_LESS 0x0201
#endif
#ifndef GL_EQUAL
#define GL_EQUAL 0x0202
#endif
#ifndef GL_LEQUAL
#define GL_LEQUAL 0x0203
#endif
#ifndef GL_GREATER
#define GL_GREATER 0x0204
#endif
#ifndef GL_NOTEQUAL
#define GL_NOTEQUAL 0x0205
#endif
#ifndef GL_GEQUAL
#define GL_GEQUAL 0x0206
#endif
#ifndef GL_ALWAYS
#define GL_ALWAYS 0x0207
#endif

#ifndef GL_KEEP
#define GL_KEEP 0x1E00
#endif
#ifndef GL_REPLACE
#define GL_REPLACE 0x1E01
#endif
#ifndef GL_INCR
#define GL_INCR 0x1E02
#endif
#ifndef GL_DECR
#define GL_DECR 0x1E03
#endif
#ifndef GL_INVERT
#define GL_INVERT 0x150A
#endif
#ifndef GL_INCR_WRAP
#define GL_INCR_WRAP 0x8507
#endif
#ifndef GL_DECR_WRAP
#define GL_DECR_WRAP 0x8508
#endif

#ifndef GL_FRONT
#define GL_FRONT 0x0404
#endif
#ifndef GL_BACK
#define GL_BACK 0x0405
#endif
#ifndef GL_FRONT_AND_BACK
#define GL_FRONT_AND_BACK 0x0408
#endif
#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif
#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif
#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif

#ifndef GL_DONT_CARE
#define GL_DONT_CARE 0x1100
#endif
#ifndef GL_FASTEST
#define GL_FASTEST 0x1101
#endif
#ifndef GL_NICEST
#define GL_NICEST 0x1102
#endif

#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_GREEN
#define GL_GREEN 0x1904
#endif
#ifndef GL_BLUE
#define GL_BLUE 0x1905
#endif
#ifndef GL_ALPHA
#define GL_ALPHA 0x1906
#endif
#ifndef GL_RGB
#define GL_RGB 0x1907
#endif
#ifndef GL_RGBA
#define GL_RGBA 0x1908
#endif
#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif
#ifndef GL_FIXED_ONLY
#define GL_FIXED_ONLY 0x891D
#endif

#ifndef GL_ARRAY_BUFFER_ARB
#define GL_ARRAY_BUFFER_ARB 0x8892
#endif
#ifndef GL_STATIC_DRAW_ARB
#define GL_STATIC_DRAW_ARB 0x88E4
#endif
#ifndef GL_BUFFER
#define GL_BUFFER 0x82E0
#endif

#ifndef GL_GEOMETRY_VERTICES_OUT_EXT
#define GL_GEOMETRY_VERTICES_OUT_EXT 0x8DDA
#endif
#ifndef GL_GEOMETRY_INPUT_TYPE_EXT
#define GL_GEOMETRY_INPUT_TYPE_EXT 0x8DDB
#endif
#ifndef GL_GEOMETRY_OUTPUT_TYPE_EXT
#define GL_GEOMETRY_OUTPUT_TYPE_EXT 0x8DDC
#endif

#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D 0x0DE0
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_TEXTURE_3D
#define GL_TEXTURE_3D 0x806F
#endif
#ifndef GL_TEXTURE_2D_ARRAY
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#endif
#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#endif
#ifndef GL_TEXTURE_RECTANGLE_ARB
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif
#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER 0x8C2A
#endif
#ifndef GL_TEXTURE_2D_MULTISAMPLE
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#endif
#ifndef GL_TEXTURE_2D_MULTISAMPLE_ARRAY
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#endif

#ifndef GL_POINTS
#define GL_POINTS 0x0000
#endif
#ifndef GL_LINES
#define GL_LINES 0x0001
#endif
#ifndef GL_LINE_STRIP
#define GL_LINE_STRIP 0x0002
#endif
#ifndef GL_LINE_LOOP
#define GL_LINE_LOOP 0x0003
#endif
#ifndef GL_TRIANGLES
#define GL_TRIANGLES 0x0004
#endif
#ifndef GL_TRIANGLE_STRIP
#define GL_TRIANGLE_STRIP 0x0005
#endif
#ifndef GL_TRIANGLE_FAN
#define GL_TRIANGLE_FAN 0x0006
#endif
#ifndef GL_QUADS
#define GL_QUADS 0x0007
#endif
#ifndef GL_QUAD_STRIP
#define GL_QUAD_STRIP 0x0008
#endif
#ifndef GL_POLYGON
#define GL_POLYGON 0x0009
#endif
#ifndef GL_LINES_ADJACENCY
#define GL_LINES_ADJACENCY 0x000A
#endif
#ifndef GL_LINE_STRIP_ADJACENCY
#define GL_LINE_STRIP_ADJACENCY 0x000B
#endif
#ifndef GL_TRIANGLES_ADJACENCY
#define GL_TRIANGLES_ADJACENCY 0x000C
#endif
#ifndef GL_TRIANGLE_STRIP_ADJACENCY
#define GL_TRIANGLE_STRIP_ADJACENCY 0x000D
#endif
#ifndef GL_PATCHES
#define GL_PATCHES 0x000E
#endif
#ifndef GL_NO_ERROR
#define GL_NO_ERROR 0
#endif
#ifndef GL_CURRENT
#define GL_CURRENT 0x0001
#endif
#ifndef GL_PROJECTION
#define GL_PROJECTION 0x1701
#endif
#ifndef GL_MODELVIEW
#define GL_MODELVIEW 0x1700
#endif
#ifndef GL_VIEWPORT
#define GL_VIEWPORT 0x0BA2
#endif
#ifndef GL_SCISSOR_TEST
#define GL_SCISSOR_TEST 0x0C11
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif
#ifndef GL_DEPTH_TEST
#define GL_DEPTH_TEST 0x0B71
#endif
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_CULL_FACE
#define GL_CULL_FACE 0x0B44
#endif
#ifndef GL_CULL_FACE_MODE
#define GL_CULL_FACE_MODE 0x0B45
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE 0x0BA1
#endif
#ifndef GL_RESCALE_NORMAL
#define GL_RESCALE_NORMAL 0x803A
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_BLEND
#define GL_BLEND 0x0BE2
#endif
#ifndef GL_OFFSET
#define GL_OFFSET 0x1E00
#endif
#ifndef GL_DECAL
#define GL_DECAL 0x2101
#endif
#ifndef GL_MODULATE
#define GL_MODULATE 0x2100
#endif
#ifndef GL_AMBIENT
#define GL_AMBIENT 0x1200
#endif
#ifndef GL_DIFFUSE
#define GL_DIFFUSE 0x1201
#endif
#ifndef GL_SPECULAR
#define GL_SPECULAR 0x1202
#endif
#ifndef GL_EMISSION
#define GL_EMISSION 0x1600
#endif
#ifndef GL_SHININESS
#define GL_SHININESS 0x1601
#endif
#ifndef GL_SRC_COLOR
#define GL_SRC_COLOR 0x0300
#endif
#ifndef GL_ONE_MINUS_SRC_COLOR
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#endif
#ifndef GL_SRC_ALPHA
#define GL_SRC_ALPHA 0x0302
#endif
#ifndef GL_ONE_MINUS_SRC_ALPHA
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#endif
#ifndef GL_DST_ALPHA
#define GL_DST_ALPHA 0x0304
#endif
#ifndef GL_ONE_MINUS_DST_ALPHA
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#endif
#ifndef GL_DST_COLOR
#define GL_DST_COLOR 0x0306
#endif
#ifndef GL_ONE_MINUS_DST_COLOR
#define GL_ONE_MINUS_DST_COLOR 0x0307
#endif
#ifndef GL_SRC_ALPHA_SATURATE
#define GL_SRC_ALPHA_SATURATE 0x0308
#endif
#ifndef GL_NEAREST
#define GL_NEAREST 0x2600
#endif
#ifndef GL_LINEAR
#define GL_LINEAR 0x2601
#endif
#ifndef GL_NEAREST_MIPMAP_NEAREST
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#endif
#ifndef GL_NEAREST_MIPMAP_LINEAR
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#endif
#ifndef GL_LINEAR_MIPMAP_NEAREST
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_REPEAT
#define GL_REPEAT 0x2901
#endif
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_CLAMP
#define GL_CLAMP 0x2900
#endif
#ifndef GL_LUMINANCE
#define GL_LUMINANCE 0x1909
#endif
#ifndef GL_LUMINANCE_ALPHA
#define GL_LUMINANCE_ALPHA 0x190A
#endif
#ifndef GL_DEPTH_COMPONENT
#define GL_DEPTH_COMPONENT 0x1902
#endif
#ifndef GL_STENCIL_INDEX
#define GL_STENCIL_INDEX 0x1901
#endif
#ifndef GL_INTENSITY
#define GL_INTENSITY 0x804C
#endif
#ifndef GL_BYTE
#define GL_BYTE 0x1400
#endif
#ifndef GL_UNSIGNED_BYTE
#define GL_UNSIGNED_BYTE 0x1401
#endif
#ifndef GL_SHORT
#define GL_SHORT 0x1402
#endif
#ifndef GL_UNSIGNED_SHORT
#define GL_UNSIGNED_SHORT 0x1403
#endif
#ifndef GL_INT
#define GL_INT 0x1404
#endif
#ifndef GL_UNSIGNED_INT
#define GL_UNSIGNED_INT 0x1405
#endif
#ifndef GL_FLOAT
#define GL_FLOAT 0x1406
#endif
#ifndef GL_DOUBLE
#define GL_DOUBLE 0x140A
#endif
#ifndef GL_HALF_FLOAT
#define GL_HALF_FLOAT 0x140B
#endif
#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif
#ifndef GL_TEXTURE_CUBE_MAP_POSITIVE_X
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_TEXTURE2
#define GL_TEXTURE2 0x84C2
#endif
#ifndef GL_TEXTURE3
#define GL_TEXTURE3 0x84C3
#endif
#ifndef GL_UNPACK_ALIGNMENT
#define GL_UNPACK_ALIGNMENT 0x0CF5
#endif
#ifndef GL_PACK_ALIGNMENT
#define GL_PACK_ALIGNMENT 0x0D05
#endif

// ---------------------------------------------------------------------------
// GL typedefs (subset used by the serialization layer)
// Defined at global scope (consistent with GL headers); the generic_osg serialization code uses GLint etc. directly in the global namespace.
// ---------------------------------------------------------------------------
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef signed char    GLbyte;
typedef short          GLshort;
typedef int            GLint;
typedef int            GLsizei;
typedef unsigned char  GLubyte;
typedef unsigned short GLushort;
typedef unsigned int   GLuint;
typedef float          GLfloat;
typedef float          GLclampf;
typedef double         GLdouble;
typedef double         GLclampd;
typedef char           GLchar;
typedef std::int64_t   GLint64;
typedef std::uint64_t  GLuint64;

namespace osg {

// ---------------------------------------------------------------------------
// Reference counting base + smart pointer
// ---------------------------------------------------------------------------
class Referenced
{
public:
    Referenced() : _refCount(0) {}
    Referenced(const Referenced&) : _refCount(0) {}
    Referenced& operator=(const Referenced&) { return *this; }
    virtual ~Referenced() {}

    void ref() const { ++_refCount; }
    void unref() const { if (--_refCount <= 0) delete this; }
    void unref_nodelete() const { --_refCount; }
    int referenceCount() const { return _refCount; }

protected:
    mutable int _refCount;
};

template<class T>
class ref_ptr
{
public:
    typedef T element_type;

    ref_ptr() : _ptr(0) {}
    ref_ptr(T* ptr) : _ptr(ptr) { if (_ptr) _ptr->ref(); }
    ref_ptr(const ref_ptr& rp) : _ptr(rp._ptr) { if (_ptr) _ptr->ref(); }
    template<class OtherT>
    ref_ptr(const ref_ptr<OtherT>& rp) : _ptr(rp.get()) { if (_ptr) _ptr->ref(); }
    ~ref_ptr() { if (_ptr) _ptr->unref(); }

    ref_ptr& operator=(const ref_ptr& rp)
    { if (_ptr != rp._ptr) { T* tmp = _ptr; _ptr = rp._ptr; if (_ptr) _ptr->ref(); if (tmp) tmp->unref(); } return *this; }
    template<class OtherT>
    ref_ptr& operator=(const ref_ptr<OtherT>& rp)
    { if (_ptr != rp.get()) { T* tmp = _ptr; _ptr = rp.get(); if (_ptr) _ptr->ref(); if (tmp) tmp->unref(); } return *this; }
    ref_ptr& operator=(T* ptr)
    { if (_ptr != ptr) { T* tmp = _ptr; _ptr = ptr; if (_ptr) _ptr->ref(); if (tmp) tmp->unref(); } return *this; }

    void swap(ref_ptr& rp) { T* tmp = _ptr; _ptr = rp._ptr; rp._ptr = tmp; }
    T* get() const { return _ptr; }
    T* release() { T* tmp = _ptr; _ptr = 0; return tmp; }
    T& operator*() const { return *_ptr; }
    T* operator->() const { return _ptr; }
    bool valid() const { return _ptr != 0; }
    explicit operator bool() const { return _ptr != 0; }
    bool operator!() const { return _ptr == 0; }

    bool operator==(const ref_ptr& rp) const { return _ptr == rp._ptr; }
    bool operator==(const T* ptr) const { return _ptr == ptr; }
    friend bool operator==(const T* lhs, const ref_ptr& rhs) { return lhs == rhs._ptr; }
    bool operator!=(const ref_ptr& rp) const { return _ptr != rp._ptr; }
    bool operator!=(const T* ptr) const { return _ptr != ptr; }
    friend bool operator!=(const T* lhs, const ref_ptr& rhs) { return lhs != rhs._ptr; }
    bool operator<(const ref_ptr& rp) const { return _ptr < rp._ptr; }

protected:
    T* _ptr;
};

// ---------------------------------------------------------------------------
// Small parsing helpers (were in osg/Math / osg/Endian)
// ---------------------------------------------------------------------------
inline float asciiToFloat(const char* s)
{
    if (!s) return 0.0f;
    return static_cast<float>(std::strtod(s, 0));
}

inline double asciiToDouble(const char* s)
{
    if (!s) return 0.0;
    return std::strtod(s, 0);
}

inline bool asciiToBool(const char* s)
{
    if (!s) return false;
    if (std::strcmp(s, "true") == 0 || std::strcmp(s, "TRUE") == 0 ||
        std::strcmp(s, "1") == 0) return true;
    return false;
}

template <class T>
inline void swapBytes(T& value)
{
    unsigned char* ptr = reinterpret_cast<unsigned char*>(&value);
    unsigned int sz = static_cast<unsigned int>(sizeof(T));
    for (unsigned int i = 0; i < sz / 2; ++i)
    {
        unsigned char t = ptr[i];
        ptr[i] = ptr[sz - i - 1];
        ptr[sz - i - 1] = t;
    }
}

// ---------------------------------------------------------------------------
// Vec<N,T> template - the osg Vec2*/Vec3*/Vec4* families
// ---------------------------------------------------------------------------
template<unsigned int N, typename T>
class Vec
{
public:
    typedef T value_type;
    T _v[N];

    Vec() { for (unsigned int i = 0; i < N; ++i) _v[i] = T(0); }

    Vec(T a) { set(a); }
    Vec(T a, T b) { set(a, b); }
    Vec(T a, T b, T c) { set(a, b, c); }
    Vec(T a, T b, T c, T d) { set(a, b, c, d); }

    template<typename A>
    void set(A a) { if (N > 0) _v[0] = static_cast<T>(a); for (unsigned int i = 1; i < N; ++i) _v[i] = T(0); }
    template<typename A, typename B>
    void set(A a, B b) { if (N > 0) _v[0] = static_cast<T>(a); if (N > 1) _v[1] = static_cast<T>(b); for (unsigned int i = 2; i < N; ++i) _v[i] = T(0); }
    template<typename A, typename B, typename C>
    void set(A a, B b, C c) { if (N > 0) _v[0] = static_cast<T>(a); if (N > 1) _v[1] = static_cast<T>(b); if (N > 2) _v[2] = static_cast<T>(c); for (unsigned int i = 3; i < N; ++i) _v[i] = T(0); }
    template<typename A, typename B, typename C, typename D>
    void set(A a, B b, C c, D d) { if (N > 0) _v[0] = static_cast<T>(a); if (N > 1) _v[1] = static_cast<T>(b); if (N > 2) _v[2] = static_cast<T>(c); if (N > 3) _v[3] = static_cast<T>(d); }

    bool operator==(const Vec& r) const
    { for (unsigned int i = 0; i < N; ++i) if (_v[i] != r._v[i]) return false; return true; }
    bool operator!=(const Vec& r) const { return !(*this == r); }

    Vec operator+(const Vec& r) const
    { Vec o; for (unsigned int i = 0; i < N; ++i) o._v[i] = _v[i] + r._v[i]; return o; }
    Vec operator-(const Vec& r) const
    { Vec o; for (unsigned int i = 0; i < N; ++i) o._v[i] = _v[i] - r._v[i]; return o; }
    Vec operator-() const
    { Vec o; for (unsigned int i = 0; i < N; ++i) o._v[i] = -_v[i]; return o; }
    Vec operator*(T s) const
    { Vec o; for (unsigned int i = 0; i < N; ++i) o._v[i] = _v[i] * s; return o; }
    Vec operator/(T s) const
    { Vec o; for (unsigned int i = 0; i < N; ++i) o._v[i] = _v[i] / s; return o; }
    Vec& operator+=(const Vec& r)
    { for (unsigned int i = 0; i < N; ++i) _v[i] += r._v[i]; return *this; }
    Vec& operator-=(const Vec& r)
    { for (unsigned int i = 0; i < N; ++i) _v[i] -= r._v[i]; return *this; }
    Vec& operator*=(T s)
    { for (unsigned int i = 0; i < N; ++i) _v[i] *= s; return *this; }
    Vec& operator/=(T s)
    { for (unsigned int i = 0; i < N; ++i) _v[i] /= s; return *this; }

    T operator[](unsigned int i) const { return _v[i]; }
    T& operator[](unsigned int i) { return _v[i]; }
    unsigned int size() const { return N; }

    T x() const { return _v[0]; }
    T y() const { return _v[1]; }
    T z() const { return _v[2]; }
    T w() const { return _v[3]; }
    T& x() { return _v[0]; }
    T& y() { return _v[1]; }
    T& z() { return _v[2]; }
    T& w() { return _v[3]; }
    T r() const { return _v[0]; }
    T g() const { return _v[1]; }
    T b() const { return _v[2]; }
    T a() const { return _v[3]; }
    T& r() { return _v[0]; }
    T& g() { return _v[1]; }
    T& b() { return _v[2]; }
    T& a() { return _v[3]; }

    double length() const
    { double l = 0; for (unsigned int i = 0; i < N; ++i) l += double(_v[i]) * double(_v[i]); return std::sqrt(l); }
};

typedef Vec<2, signed char>        Vec2b;
typedef Vec<2, unsigned char>      Vec2ub;
typedef Vec<2, short>              Vec2s;
typedef Vec<2, unsigned short>     Vec2us;
typedef Vec<2, int>                Vec2i;
typedef Vec<2, unsigned int>       Vec2ui;
typedef Vec<2, float>              Vec2f;
typedef Vec<2, double>             Vec2d;
typedef Vec<2, float>              Vec2;

typedef Vec<3, signed char>        Vec3b;
typedef Vec<3, unsigned char>      Vec3ub;
typedef Vec<3, short>              Vec3s;
typedef Vec<3, unsigned short>     Vec3us;
typedef Vec<3, int>                Vec3i;
typedef Vec<3, unsigned int>       Vec3ui;
typedef Vec<3, float>              Vec3f;
typedef Vec<3, double>             Vec3d;
typedef Vec<3, float>              Vec3;

typedef Vec<4, signed char>        Vec4b;
typedef Vec<4, unsigned char>      Vec4ub;
typedef Vec<4, short>              Vec4s;
typedef Vec<4, unsigned short>     Vec4us;
typedef Vec<4, int>                Vec4i;
typedef Vec<4, unsigned int>       Vec4ui;
typedef Vec<4, float>              Vec4f;
typedef Vec<4, double>             Vec4d;
typedef Vec<4, float>              Vec4;

template<unsigned int N, typename T>
inline std::ostream& operator<<(std::ostream& out, const Vec<N, T>& v)
{
    out << "(";
    for (unsigned int i = 0; i < N; ++i)
    {
        if (i) out << ", ";
        out << v._v[i];
    }
    out << ")";
    return out;
}

// ---------------------------------------------------------------------------
// Quat / Plane / Matrix / Matrixf
// ---------------------------------------------------------------------------
class Quat
{
public:
    double _v[4];
    Quat() { _v[0] = 0.0; _v[1] = 0.0; _v[2] = 0.0; _v[3] = 1.0; }
    Quat(double x, double y, double z, double w) { _v[0] = x; _v[1] = y; _v[2] = z; _v[3] = w; }
    double x() const { return _v[0]; }
    double y() const { return _v[1]; }
    double z() const { return _v[2]; }
    double w() const { return _v[3]; }
    bool operator==(const Quat& r) const
    { return _v[0] == r._v[0] && _v[1] == r._v[1] && _v[2] == r._v[2] && _v[3] == r._v[3]; }
    bool operator!=(const Quat& r) const { return !(*this == r); }
};

inline std::ostream& operator<<(std::ostream& out, const Quat& q)
{
    out << "Quat(" << q._v[0] << ", " << q._v[1] << ", " << q._v[2] << ", " << q._v[3] << ")";
    return out;
}

class Plane
{
public:
    double _v[4];
    Plane() { _v[0] = _v[1] = _v[2] = _v[3] = 0.0; }
    Plane(double a, double b, double c, double d) { _v[0] = a; _v[1] = b; _v[2] = c; _v[3] = d; }
};

class Matrixd
{
public:
    typedef double value_type;
    double _m[16];
    Matrixd() { makeIdentity(); }
    void makeIdentity()
    { for (int i = 0; i < 16; ++i) _m[i] = 0.0; _m[0] = _m[5] = _m[10] = _m[15] = 1.0; }
    double& operator()(int row, int col) { return _m[col * 4 + row]; }
    double operator()(int row, int col) const { return _m[col * 4 + row]; }
    double& operator[](int i) { return _m[i]; }
    double operator[](int i) const { return _m[i]; }
    bool operator==(const Matrixd& r) const
    { for (int i = 0; i < 16; ++i) if (_m[i] != r._m[i]) return false; return true; }
    bool operator!=(const Matrixd& r) const { return !(*this == r); }
};

class Matrixf
{
public:
    typedef float value_type;
    float _m[16];
    Matrixf() { makeIdentity(); }
    void makeIdentity()
    { for (int i = 0; i < 16; ++i) _m[i] = 0.0f; _m[0] = _m[5] = _m[10] = _m[15] = 1.0f; }
    float& operator()(int row, int col) { return _m[col * 4 + row]; }
    float operator()(int row, int col) const { return _m[col * 4 + row]; }
    float& operator[](int i) { return _m[i]; }
    float operator[](int i) const { return _m[i]; }
    bool operator==(const Matrixf& r) const
    { for (int i = 0; i < 16; ++i) if (_m[i] != r._m[i]) return false; return true; }
    bool operator!=(const Matrixf& r) const { return !(*this == r); }
};

typedef Matrixd Matrix;

inline std::ostream& operator<<(std::ostream& out, const Matrixd& m)
{
    out << "Matrix(";
    for (int i = 0; i < 16; ++i)
    {
        if (i) out << ", ";
        out << m._m[i];
    }
    out << ")";
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const Matrixf& m)
{
    out << "Matrixf(";
    for (int i = 0; i < 16; ++i)
    {
        if (i) out << ", ";
        out << m._m[i];
    }
    out << ")";
    return out;
}

// ---------------------------------------------------------------------------
// Bounding box / sphere
// ---------------------------------------------------------------------------
template<typename T>
class BoundingBoxImpl
{
public:
    Vec<3, T> _min, _max;
    BoundingBoxImpl() { init(); }
    BoundingBoxImpl(const Vec<3, T>& min, const Vec<3, T>& max) : _min(min), _max(max) {}
    void init()
    {
        for (unsigned int i = 0; i < 3; ++i)
        {
            _min._v[i] = std::numeric_limits<T>::max();
            _max._v[i] = -std::numeric_limits<T>::max();
        }
    }
    bool valid() const { return _min._v[0] <= _max._v[0] && _min._v[1] <= _max._v[1] && _min._v[2] <= _max._v[2]; }
};

typedef BoundingBoxImpl<float>  BoundingBoxf;
typedef BoundingBoxImpl<double> BoundingBoxd;
typedef BoundingBoxImpl<double> BoundingBox;

template<typename T>
class BoundingSphereImpl
{
public:
    Vec<3, T> _center;
    T _radius;
    BoundingSphereImpl() : _radius(0.0) { _center._v[0] = _center._v[1] = _center._v[2] = T(0); }
    BoundingSphereImpl(const Vec<3, T>& center, T radius) : _center(center), _radius(radius) {}
    bool valid() const { return _radius >= T(0); }
    T radius() const { return _radius; }
    const Vec<3, T>& center() const { return _center; }
};

typedef BoundingSphereImpl<float>  BoundingSpheref;
typedef BoundingSphereImpl<double> BoundingSphered;
typedef BoundingSphereImpl<double> BoundingSphere;

// ---------------------------------------------------------------------------
// Minimal osg::StateSet stub - only the members referenced by the
// hand-written generic_osg serializers (readModeList / readTextureModeList).
// ---------------------------------------------------------------------------
class StateSet : public Referenced
{
public:
    typedef std::map<GLenum, GLint> ModeList;
    static const int DEFAULT_BIN = 0;
};

} // namespace osg

#endif // OSGVERSE_GENERIC_WRAPPERS_GENERIC_MATH_H
