#include "GenericInputStream.h"
#include "GenericReserializer.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <mutex>
#include <stdarg.h>

using namespace osgVerse;

const std::string IntLookup::_empty = "";

// ---------------------------------------------------------------------------
// Local ASCII parsing helpers (not depending on the exact tool signatures in GenericMath.h)
// ---------------------------------------------------------------------------
namespace
{
    inline int g_asciiToInt(const char* s) { return (int)strtol(s, NULL, 0); }
    inline float g_asciiToFloat(const char* s) { return (float)strtod(s, NULL); }
    inline double g_asciiToDouble(const char* s) { return strtod(s, NULL); }

    inline void g_swapBytes(char* ptr, unsigned int size)
    {
        for (unsigned int i = 0; i < size / 2; ++i)
        { std::swap(ptr[i], ptr[size - 1 - i]); }
    }

    // Map the compact ArrayType id to the osg class name / component count so a
    // visitor or the IR can rebuild the array (legacy files carry no class name).
    inline const char* g_osgArrayClass(unsigned int t)
    {
        switch (t)
        {
        case ID_BYTE_ARRAY:   return "osg::ByteArray";
        case ID_UBYTE_ARRAY:  return "osg::UByteArray";
        case ID_SHORT_ARRAY:  return "osg::ShortArray";
        case ID_USHORT_ARRAY: return "osg::UShortArray";
        case ID_INT_ARRAY:    return "osg::IntArray";
        case ID_UINT_ARRAY:   return "osg::UIntArray";
        case ID_FLOAT_ARRAY:  return "osg::FloatArray";
        case ID_DOUBLE_ARRAY: return "osg::DoubleArray";
        case ID_VEC2B_ARRAY:  return "osg::Vec2bArray";
        case ID_VEC3B_ARRAY:  return "osg::Vec3bArray";
        case ID_VEC4B_ARRAY:  return "osg::Vec4bArray";
        case ID_VEC4UB_ARRAY: return "osg::Vec4ubArray";
        case ID_VEC2S_ARRAY:  return "osg::Vec2sArray";
        case ID_VEC3S_ARRAY:  return "osg::Vec3sArray";
        case ID_VEC4S_ARRAY:  return "osg::Vec4sArray";
        case ID_VEC2_ARRAY:   return "osg::Vec2Array";
        case ID_VEC3_ARRAY:   return "osg::Vec3Array";
        case ID_VEC4_ARRAY:   return "osg::Vec4Array";
        case ID_VEC2D_ARRAY:  return "osg::Vec2dArray";
        case ID_VEC3D_ARRAY:  return "osg::Vec3dArray";
        case ID_VEC4D_ARRAY:  return "osg::Vec4dArray";
        case ID_VEC2UB_ARRAY: return "osg::Vec2ubArray";
        case ID_VEC3UB_ARRAY: return "osg::Vec3ubArray";
        case ID_VEC2US_ARRAY: return "osg::Vec2usArray";
        case ID_VEC3US_ARRAY: return "osg::Vec3usArray";
        case ID_VEC4US_ARRAY: return "osg::Vec4usArray";
        case ID_VEC2I_ARRAY:  return "osg::Vec2iArray";
        case ID_VEC3I_ARRAY:  return "osg::Vec3iArray";
        case ID_VEC4I_ARRAY:  return "osg::Vec4iArray";
        case ID_VEC2UI_ARRAY: return "osg::Vec2uiArray";
        case ID_VEC3UI_ARRAY: return "osg::Vec3uiArray";
        case ID_VEC4UI_ARRAY: return "osg::Vec4uiArray";
        case ID_UINT64_ARRAY: return "osg::UInt64Array";
        case ID_INT64_ARRAY:  return "osg::Int64Array";
        default: break;
        }
        return "osg::Array";
    }

    inline int g_arrayComponents(unsigned int t)
    {
        switch (t)
        {
        case ID_VEC2B_ARRAY: case ID_VEC2S_ARRAY: case ID_VEC2_ARRAY: case ID_VEC2D_ARRAY:
        case ID_VEC2UB_ARRAY: case ID_VEC2US_ARRAY: case ID_VEC2I_ARRAY: case ID_VEC2UI_ARRAY:
            return 2;
        case ID_VEC3B_ARRAY: case ID_VEC3S_ARRAY: case ID_VEC3_ARRAY: case ID_VEC3D_ARRAY:
        case ID_VEC3UB_ARRAY: case ID_VEC3US_ARRAY: case ID_VEC3I_ARRAY: case ID_VEC3UI_ARRAY:
            return 3;
        case ID_VEC4B_ARRAY: case ID_VEC4UB_ARRAY: case ID_VEC4S_ARRAY: case ID_VEC4_ARRAY:
        case ID_VEC4D_ARRAY: case ID_VEC4US_ARRAY: case ID_VEC4I_ARRAY: case ID_VEC4UI_ARRAY:
            return 4;
        default: return 1;
        }
    }

    // ------------------------------------------------------------------
    // Default lookup tables (ArrayType / PrimitiveType / ObjectType / GL)
    // ------------------------------------------------------------------
    UserLookupTableProxy::LookupTable createDefaultTables()
    {
        UserLookupTableProxy::LookupTable tables;

        IntLookup& arrayType = tables["ArrayType"];
        arrayType.add("ByteArray", ID_BYTE_ARRAY);
        arrayType.add("UByteArray", ID_UBYTE_ARRAY);
        arrayType.add("ShortArray", ID_SHORT_ARRAY);
        arrayType.add("UShortArray", ID_USHORT_ARRAY);
        arrayType.add("IntArray", ID_INT_ARRAY);
        arrayType.add("UIntArray", ID_UINT_ARRAY);
        arrayType.add("FloatArray", ID_FLOAT_ARRAY);
        arrayType.add("DoubleArray", ID_DOUBLE_ARRAY);
        arrayType.add("Vec2bArray", ID_VEC2B_ARRAY);
        arrayType.add("Vec3bArray", ID_VEC3B_ARRAY);
        arrayType.add("Vec4bArray", ID_VEC4B_ARRAY);
        arrayType.add("Vec4ubArray", ID_VEC4UB_ARRAY);
        arrayType.add("Vec2sArray", ID_VEC2S_ARRAY);
        arrayType.add("Vec3sArray", ID_VEC3S_ARRAY);
        arrayType.add("Vec4sArray", ID_VEC4S_ARRAY);
        arrayType.add("Vec2Array", ID_VEC2_ARRAY);
        arrayType.add("Vec2fArray", ID_VEC2_ARRAY);
        arrayType.add("Vec3Array", ID_VEC3_ARRAY);
        arrayType.add("Vec3fArray", ID_VEC3_ARRAY);
        arrayType.add("Vec4Array", ID_VEC4_ARRAY);
        arrayType.add("Vec4fArray", ID_VEC4_ARRAY);
        arrayType.add("Vec2dArray", ID_VEC2D_ARRAY);
        arrayType.add("Vec3dArray", ID_VEC3D_ARRAY);
        arrayType.add("Vec4dArray", ID_VEC4D_ARRAY);
        arrayType.add("Vec2ubArray", ID_VEC2UB_ARRAY);
        arrayType.add("Vec3ubArray", ID_VEC3UB_ARRAY);
        arrayType.add("Vec2usArray", ID_VEC2US_ARRAY);
        arrayType.add("Vec3usArray", ID_VEC3US_ARRAY);
        arrayType.add("Vec4usArray", ID_VEC4US_ARRAY);
        arrayType.add("Vec2iArray", ID_VEC2I_ARRAY);
        arrayType.add("Vec3iArray", ID_VEC3I_ARRAY);
        arrayType.add("Vec4iArray", ID_VEC4I_ARRAY);
        arrayType.add("Vec2uiArray", ID_VEC2UI_ARRAY);
        arrayType.add("Vec3uiArray", ID_VEC3UI_ARRAY);
        arrayType.add("Vec4uiArray", ID_VEC4UI_ARRAY);
        arrayType.add("UInt64Array", ID_UINT64_ARRAY);
        arrayType.add("Int64Array", ID_INT64_ARRAY);

        IntLookup& primType = tables["PrimitiveType"];
        primType.add("DrawArrays", ID_DRAWARRAYS);
        primType.add("DrawArraysLength", ID_DRAWARRAY_LENGTH);
        primType.add("DrawArrayLengths", ID_DRAWARRAY_LENGTH);  // tolerant alias
        primType.add("DrawElementsUByte", ID_DRAWELEMENTS_UBYTE);
        primType.add("DrawElementsUShort", ID_DRAWELEMENTS_USHORT);
        primType.add("DrawElementsUInt", ID_DRAWELEMENTS_UINT);
        primType.add("GL_POINTS", GL_POINTS);
        primType.add("GL_LINES", GL_LINES);
        primType.add("GL_LINE_STRIP", GL_LINE_STRIP);
        primType.add("GL_LINE_LOOP", GL_LINE_LOOP);
        primType.add("GL_TRIANGLES", GL_TRIANGLES);
        primType.add("GL_TRIANGLE_STRIP", GL_TRIANGLE_STRIP);
        primType.add("GL_TRIANGLE_FAN", GL_TRIANGLE_FAN);
        primType.add("GL_QUADS", GL_QUADS);
        primType.add("GL_QUAD_STRIP", GL_QUAD_STRIP);
        primType.add("GL_POLYGON", GL_POLYGON);
        primType.add("GL_LINES_ADJACENCY", GL_LINES_ADJACENCY);
        primType.add("GL_LINE_STRIP_ADJACENCY", GL_LINE_STRIP_ADJACENCY);
        primType.add("GL_TRIANGLES_ADJACENCY", GL_TRIANGLES_ADJACENCY);
        primType.add("GL_TRIANGLE_STRIP_ADJACENCY", GL_TRIANGLE_STRIP_ADJACENCY);
        primType.add("GL_PATCHES", GL_PATCHES);

        IntLookup& objType = tables["ObjectType"];
        objType.add("osg::Object", 1);
        objType.add("osg::Node", 2);
        objType.add("osg::Group", 3);
        objType.add("osg::Geode", 4);
        objType.add("osg::Geometry", 5);
        objType.add("osg::StateSet", 6);
        objType.add("osg::Image", 7);
        objType.add("osg::Array", 8);
        objType.add("osg::PrimitiveSet", 9);
        objType.add("osg::Drawable", 10);
        objType.add("osg::Transform", 11);
        objType.add("osg::MatrixTransform", 12);
        objType.add("osg::PositionAttitudeTransform", 13);
        objType.add("osg::Camera", 14);
        objType.add("osg::Viewport", 15);
        objType.add("osg::Texture", 16);
        objType.add("osg::Texture2D", 17);
        objType.add("osg::Material", 18);
        objType.add("osg::Light", 19);
        objType.add("osg::LightSource", 20);
        objType.add("osg::LOD", 21);
        objType.add("osg::Switch", 22);
        objType.add("osg::AutoTransform", 23);
        objType.add("osg::Billboard", 24);
        objType.add("osg::Sequence", 25);
        objType.add("osg::Uniform", 26);
        objType.add("osg::Program", 27);
        objType.add("osg::Shader", 28);
        objType.add("osg::TextureCubeMap", 29);
        objType.add("osg::TextureRectangle", 30);
        objType.add("osg::Texture1D", 31);
        objType.add("osg::Texture3D", 32);
        objType.add("osg::Point", 33);
        objType.add("osg::LineWidth", 34);
        objType.add("osg::PolygonMode", 35);
        objType.add("osg::PolygonOffset", 36);
        objType.add("osg::BlendFunc", 37);
        objType.add("osg::CullFace", 38);
        objType.add("osg::FrontFace", 39);
        objType.add("osg::Depth", 40);
        objType.add("osg::Stencil", 41);
        objType.add("osg::AlphaFunc", 42);
        objType.add("osg::ColorMask", 43);
        objType.add("osg::Scissor", 44);
        objType.add("osg::ShadeModel", 45);
        objType.add("osg::Hint", 46);
        objType.add("osg::Fog", 47);
        objType.add("osg::TexEnv", 48);
        objType.add("osg::TexGen", 49);
        objType.add("osg::TexMat", 50);
        objType.add("osg::LightModel", 51);
        objType.add("osg::ClipNode", 52);
        objType.add("osg::ClipPlane", 53);
        objType.add("osg::Projection", 54);
        objType.add("osg::CoordinateSystemNode", 55);
        objType.add("osg::Terrain", 56);
        objType.add("osg::DefaultUserDataContainer", 57);
        objType.add("osg::UserDataContainer", 58);
        objType.add("osg::Vec2fArray", 60);
        objType.add("osg::Vec3fArray", 61);
        objType.add("osg::Vec4fArray", 62);
        objType.add("osg::Vec2dArray", 63);
        objType.add("osg::Vec3dArray", 64);
        objType.add("osg::Vec4dArray", 65);

        IntLookup& gl = tables["GL"];
        gl.add("0", 0);
        gl.add("GL_NO_ERROR", GL_NO_ERROR);
        gl.add("GL_NONE", GL_NONE);
        gl.add("GL_ZERO", GL_ZERO);
        gl.add("GL_ONE", GL_ONE);
        gl.add("GL_POINTS", GL_POINTS);
        gl.add("GL_LINES", GL_LINES);
        gl.add("GL_LINE_STRIP", GL_LINE_STRIP);
        gl.add("GL_LINE_LOOP", GL_LINE_LOOP);
        gl.add("GL_TRIANGLES", GL_TRIANGLES);
        gl.add("GL_TRIANGLE_STRIP", GL_TRIANGLE_STRIP);
        gl.add("GL_TRIANGLE_FAN", GL_TRIANGLE_FAN);
        gl.add("GL_QUADS", GL_QUADS);
        gl.add("GL_QUAD_STRIP", GL_QUAD_STRIP);
        gl.add("GL_POLYGON", GL_POLYGON);
        gl.add("GL_PATCHES", GL_PATCHES);
        gl.add("GL_BYTE", GL_BYTE);
        gl.add("GL_UNSIGNED_BYTE", GL_UNSIGNED_BYTE);
        gl.add("GL_SHORT", GL_SHORT);
        gl.add("GL_UNSIGNED_SHORT", GL_UNSIGNED_SHORT);
        gl.add("GL_INT", GL_INT);
        gl.add("GL_UNSIGNED_INT", GL_UNSIGNED_INT);
        gl.add("GL_FLOAT", GL_FLOAT);
        gl.add("GL_DOUBLE", GL_DOUBLE);
        gl.add("GL_HALF_FLOAT", GL_HALF_FLOAT);
        gl.add("GL_RGBA", GL_RGBA);
        gl.add("GL_RGB", GL_RGB);
        gl.add("GL_LUMINANCE", GL_LUMINANCE);
        gl.add("GL_LUMINANCE_ALPHA", GL_LUMINANCE_ALPHA);
        gl.add("GL_ALPHA", GL_ALPHA);
        gl.add("GL_DEPTH_COMPONENT", GL_DEPTH_COMPONENT);
        gl.add("GL_STENCIL_INDEX", GL_STENCIL_INDEX);
        gl.add("GL_INTENSITY", GL_INTENSITY);
        gl.add("GL_RED", GL_RED);
        gl.add("GL_GREEN", GL_GREEN);
        gl.add("GL_BLUE", GL_BLUE);
        gl.add("GL_TEXTURE_1D", GL_TEXTURE_1D);
        gl.add("GL_TEXTURE_2D", GL_TEXTURE_2D);
        gl.add("GL_TEXTURE_3D", GL_TEXTURE_3D);
        gl.add("GL_TEXTURE_2D_ARRAY", GL_TEXTURE_2D_ARRAY);
        gl.add("GL_TEXTURE_RECTANGLE", GL_TEXTURE_RECTANGLE);
        gl.add("GL_TEXTURE_CUBE_MAP", GL_TEXTURE_CUBE_MAP);
        gl.add("GL_TEXTURE_CUBE_MAP_POSITIVE_X", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
        gl.add("GL_NEAREST", GL_NEAREST);
        gl.add("GL_LINEAR", GL_LINEAR);
        gl.add("GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST);
        gl.add("GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR);
        gl.add("GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST);
        gl.add("GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR);
        gl.add("GL_REPEAT", GL_REPEAT);
        gl.add("GL_CLAMP_TO_EDGE", GL_CLAMP_TO_EDGE);
        gl.add("GL_CLAMP", GL_CLAMP);
        gl.add("GL_DEPTH_TEST", GL_DEPTH_TEST);
        gl.add("GL_LIGHTING", GL_LIGHTING);
        gl.add("GL_BLEND", GL_BLEND);
        gl.add("GL_CULL_FACE", GL_CULL_FACE);
        gl.add("GL_CULL_FACE_MODE", GL_CULL_FACE_MODE);
        gl.add("GL_NORMALIZE", GL_NORMALIZE);
        gl.add("GL_RESCALE_NORMAL", GL_RESCALE_NORMAL);
        gl.add("GL_FOG", GL_FOG);
        gl.add("GL_SCISSOR_TEST", GL_SCISSOR_TEST);
        gl.add("GL_MULTISAMPLE", GL_MULTISAMPLE);
        gl.add("GL_FRONT", GL_FRONT);
        gl.add("GL_BACK", GL_BACK);
        gl.add("GL_FRONT_AND_BACK", GL_FRONT_AND_BACK);
        gl.add("GL_AMBIENT", GL_AMBIENT);
        gl.add("GL_DIFFUSE", GL_DIFFUSE);
        gl.add("GL_SPECULAR", GL_SPECULAR);
        gl.add("GL_EMISSION", GL_EMISSION);
        gl.add("GL_SHININESS", GL_SHININESS);
        gl.add("GL_DECAL", GL_DECAL);
        gl.add("GL_MODULATE", GL_MODULATE);
        gl.add("GL_REPLACE", GL_REPLACE);
        gl.add("GL_SRC_COLOR", GL_SRC_COLOR);
        gl.add("GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR);
        gl.add("GL_SRC_ALPHA", GL_SRC_ALPHA);
        gl.add("GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA);
        gl.add("GL_DST_ALPHA", GL_DST_ALPHA);
        gl.add("GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA);
        gl.add("GL_DST_COLOR", GL_DST_COLOR);
        gl.add("GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR);
        gl.add("GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE);
        gl.add("GL_OFFSET", GL_OFFSET);
        gl.add("GL_TEXTURE0", GL_TEXTURE0);
        gl.add("GL_TEXTURE1", GL_TEXTURE1);
        gl.add("GL_TEXTURE2", GL_TEXTURE2);
        gl.add("GL_TEXTURE3", GL_TEXTURE3);
        gl.add("GL_UNPACK_ALIGNMENT", GL_UNPACK_ALIGNMENT);
        gl.add("GL_PACK_ALIGNMENT", GL_PACK_ALIGNMENT);
        gl.add("GL_DEPTH_BUFFER_BIT", GL_DEPTH_BUFFER_BIT);
        gl.add("GL_COLOR_BUFFER_BIT", GL_COLOR_BUFFER_BIT);
        gl.add("GL_CURRENT", GL_CURRENT);
        gl.add("GL_PROJECTION", GL_PROJECTION);
        gl.add("GL_MODELVIEW", GL_MODELVIEW);
        gl.add("GL_VIEWPORT", GL_VIEWPORT);

        return tables;
    }
}

// ---------------------------------------------------------------------------
// UserLookupTableProxy
// ---------------------------------------------------------------------------
UserLookupTableProxy::LookupTable& UserLookupTableProxy::instance()
{
    static LookupTable s_tables = createDefaultTables();
    return s_tables;
}

IntLookup& UserLookupTableProxy::get(const char* name)
{
    // The registry is process-wide and the first lookup of a name inserts it
    // (the USER_READ_FUNC tables register themselves lazily). Callers may read
    // several files at once (e.g. VSG's DatabasePager uses 4 threads), so a
    // bare operator[] would let two readers reallocate/rebalance the map
    // concurrently and corrupt the heap. std::map nodes are stable, hence the
    // reference handed out here remains valid for readers that already hold it.
    static std::mutex s_mutex;
    std::lock_guard<std::mutex> lock(s_mutex);
    return instance()[name];
}

// ---------------------------------------------------------------------------
// InputIterator
// ---------------------------------------------------------------------------
InputIterator::InputIterator()
    : _in(NULL), _inputStream(NULL), _byteSwap(0),
      _supportBinaryBrackets(false), _failed(false) {}

InputIterator::~InputIterator() {}

void InputIterator::checkStream() const
{
    if (_in && _in->fail()) _failed = true;
}

void InputIterator::throwException(const std::string& msg)
{
    throw InputException(msg);
}

void InputIterator::readComponentArray(char* s, unsigned int numElements,
                                       unsigned int numComponentsPerElement,
                                       unsigned int componentSize)
{
    unsigned int size = numElements * numComponentsPerElement * componentSize;
    if (size > 0)
    {
        readCharArray(s, size);
        if (_byteSwap && componentSize > 1)
        {
            char* ptr = s;
            for (unsigned int i = 0; i < numElements * numComponentsPerElement; ++i)
            {
                g_swapBytes(ptr, componentSize);
                ptr += componentSize;
            }
        }
    }
    checkStream();
}

// ---------------------------------------------------------------------------
// AsciiInputIterator
// ---------------------------------------------------------------------------
void AsciiInputIterator::readStringImpl(std::string& str)
{
    char ch = 0;
    bool found_string = false;
    bool found_quote = false;
    str.clear();

    while (!_in->eof())
    {
        _in->get(ch);
        if (!found_string)
        {
            // Skip whitespace
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
            {
                if (found_quote) break;
                continue;
            }
            found_string = true;
            if (ch == '"') { found_quote = true; continue; }
        }
        else
        {
            if (found_quote)
            {
                if (ch == '"') break;
            }
            else
            {
                if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') break;
            }
        }
        str += ch;
    }
    checkStream();
}

bool AsciiInputIterator::isSpecialToken(const std::string& str)
{ return str == "{" || str == "}"; }

void AsciiInputIterator::readString(std::string& str)
{
    if (!_preReadString.empty())
    {
        str = _preReadString;
        _preReadString.clear();
        return;
    }
    readStringImpl(str);
}

bool AsciiInputIterator::matchString(const std::string& str)
{
    if (!_preReadString.empty())
    {
        if (_preReadString == str) { _preReadString.clear(); return true; }
        return false;
    }
    std::string readStr;
    readStringImpl(readStr);
    if (readStr == str) return true;
    _preReadString = readStr;
    return false;
}

void AsciiInputIterator::readBool(bool& b)
{
    std::string str;
    readString(str);
    b = (str == "TRUE" || str == "True" || str == "true" || str == "1");
}

void AsciiInputIterator::readChar(char& c)
{
    std::string str;
    readString(str);
    if (!str.empty()) c = str[0];
}

void AsciiInputIterator::readSChar(signed char& c)
{
    std::string str;
    readString(str);
    c = (signed char)g_asciiToInt(str.c_str());
}

void AsciiInputIterator::readUChar(unsigned char& c)
{
    std::string str;
    readString(str);
    c = (unsigned char)g_asciiToInt(str.c_str());
}

void AsciiInputIterator::readShort(short& s)
{
    std::string str;
    readString(str);
    s = (short)g_asciiToInt(str.c_str());
}

void AsciiInputIterator::readUShort(unsigned short& s)
{
    std::string str;
    readString(str);
    s = (unsigned short)g_asciiToInt(str.c_str());
}

void AsciiInputIterator::readInt(int& i)
{
    std::string str;
    readString(str);
    i = g_asciiToInt(str.c_str());
}

void AsciiInputIterator::readUInt(unsigned int& i)
{
    std::string str;
    readString(str);
    i = (unsigned int)g_asciiToInt(str.c_str());
}

void AsciiInputIterator::readLong(long& l)
{
    std::string str;
    readString(str);
    l = (long)strtol(str.c_str(), NULL, 0);
}

void AsciiInputIterator::readULong(unsigned long& l)
{
    std::string str;
    readString(str);
    l = (unsigned long)strtoul(str.c_str(), NULL, 0);
}

void AsciiInputIterator::readFloat(float& f)
{
    std::string str;
    readString(str);
    f = g_asciiToFloat(str.c_str());
}

void AsciiInputIterator::readDouble(double& d)
{
    std::string str;
    readString(str);
    d = g_asciiToDouble(str.c_str());
}

void AsciiInputIterator::readGLenum(ObjectGLenum& value)
{
    unsigned int e = 0;
    std::string enumString;
    readString(enumString);
    e = UserLookupTableProxy::get("GL").value(enumString);
    value.set(e);
    value.setType("GL");
}

void AsciiInputIterator::readProperty(ObjectProperty& prop)
{
    int value = 0;
    std::string enumString;
    readString(enumString);
    if (prop._mapProperty)
    {
        IntLookup& table = UserLookupTableProxy::get(prop._name.c_str());
        if (table.hasName(enumString))
            value = table.value(enumString);
        else
            prop._name = enumString;  // fallback: keep token name when table is not registered
    }
    else
    {
        if (prop._name != enumString)
            std::cerr << "[InputStream] Unmatched property " << enumString
                      << ", expecting " << prop._name << std::endl;
        prop._name = enumString;
    }
    prop.setValue(value);
}

void AsciiInputIterator::readMark(ObjectMark& mark)
{
    std::string markString;
    readString(markString);
    (void)mark;
}

void AsciiInputIterator::readWrappedString(std::string& str)
{
    str.clear();

    // Consume the pre-read buffer first (may come from a failed matchString)
    if (!_preReadString.empty())
    {
        str = _preReadString;
        _preReadString.clear();
        return;
    }

    char ch = 0;
    // Skip whitespace
    while (!_in->eof())
    {
        _in->get(ch);
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') break;
    }
    if (_in->eof()) { checkStream(); return; }

    if (ch == '"')
    {
        // Quoted string: read until the closing quote, handling backslash escapes
        while (!_in->eof())
        {
            _in->get(ch);
            if (ch == '\\') { _in->get(ch); str += ch; continue; }
            if (ch == '"') break;
            str += ch;
        }
    }
    else
    {
        // Unquoted string: read until whitespace
        str += ch;
        while (!_in->eof())
        {
            _in->get(ch);
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') break;
            str += ch;
        }
    }
    checkStream();
}

void AsciiInputIterator::advanceToCurrentEndBracket()
{
    // Read by token and track { } nesting (readString will first consume the pre-read buffer)
    std::string passString;
    unsigned int blocks = 0;
    while (!_in->eof())
    {
        passString.clear();
        readString(passString);
        if (passString == "}")
        {
            if (blocks <= 0) return;
            else blocks--;
        }
        else if (passString == "{") blocks++;
    }
    checkStream();
}

// ---------------------------------------------------------------------------
// BinaryInputIterator
// ---------------------------------------------------------------------------
void BinaryInputIterator::readBool(bool& b)
{
    char c = 0;
    _in->read(&c, CHAR_SIZE);
    b = (c != 0);
    checkStream();
}

void BinaryInputIterator::readChar(char& c)
{
    _in->read(&c, CHAR_SIZE);
    checkStream();
}

void BinaryInputIterator::readSChar(signed char& c)
{
    _in->read((char*)&c, CHAR_SIZE);
    checkStream();
}

void BinaryInputIterator::readUChar(unsigned char& c)
{
    _in->read((char*)&c, CHAR_SIZE);
    checkStream();
}

void BinaryInputIterator::readShort(short& s)
{
    _in->read((char*)&s, SHORT_SIZE);
    if (_byteSwap) g_swapBytes((char*)&s, SHORT_SIZE);
    checkStream();
}

void BinaryInputIterator::readUShort(unsigned short& s)
{
    _in->read((char*)&s, SHORT_SIZE);
    if (_byteSwap) g_swapBytes((char*)&s, SHORT_SIZE);
    checkStream();
}

void BinaryInputIterator::readInt(int& i)
{
    _in->read((char*)&i, INT_SIZE);
    if (_byteSwap) g_swapBytes((char*)&i, INT_SIZE);
    checkStream();
}

void BinaryInputIterator::readUInt(unsigned int& i)
{
    _in->read((char*)&i, INT_SIZE);
    if (_byteSwap) g_swapBytes((char*)&i, INT_SIZE);
    checkStream();
}

void BinaryInputIterator::readLong(long& l)
{
    int32_t value;
    _in->read((char*)&value, LONG_SIZE);
    if (_byteSwap) g_swapBytes((char*)&value, LONG_SIZE);
    l = (long)value;
    checkStream();
}

void BinaryInputIterator::readULong(unsigned long& l)
{
    uint32_t value;
    _in->read((char*)&value, LONG_SIZE);
    if (_byteSwap) g_swapBytes((char*)&value, LONG_SIZE);
    l = (unsigned long)value;
    checkStream();
}

void BinaryInputIterator::readFloat(float& f)
{
    _in->read((char*)&f, FLOAT_SIZE);
    if (_byteSwap) g_swapBytes((char*)&f, FLOAT_SIZE);
    checkStream();
}

void BinaryInputIterator::readDouble(double& d)
{
    _in->read((char*)&d, DOUBLE_SIZE);
    if (_byteSwap) g_swapBytes((char*)&d, DOUBLE_SIZE);
    checkStream();
}

void BinaryInputIterator::readString(std::string& s)
{
    int size = 0;
    readInt(size);
    if (size > 0)
    {
        s.resize(size);
        if (size > 0) _in->read(&s[0], size);
        checkStream();
    }
    else if (size < 0)
    {
        throwException("BinaryInputIterator::readString() error, negative string size read.");
    }
    else
    {
        s.clear();
    }
}

void BinaryInputIterator::readGLenum(ObjectGLenum& value)
{
    unsigned int e = 0;
    _in->read((char*)&e, GLENUM_SIZE);
    if (_byteSwap) g_swapBytes((char*)&e, GLENUM_SIZE);
    value.set(e);
    checkStream();
}

void BinaryInputIterator::readProperty(ObjectProperty& prop)
{
    int value = 0;
    if (prop._mapProperty)
    {
        _in->read((char*)&value, INT_SIZE);
        if (_byteSwap) g_swapBytes((char*)&value, INT_SIZE);
    }
    prop.setValue(value);
    checkStream();
}

void BinaryInputIterator::readMark(ObjectMark& mark)
{
    if (_supportBinaryBrackets)
    {
        if (mark._name == "{")
        {
            _beginPositions.push_back(_in->tellg());
            if (getInputStream() && getInputStream()->getFileVersion() > 148)
            {
                uint64_t size = 0;
                _in->read((char*)&size, INT64_SIZE);
                if (_byteSwap) g_swapBytes((char*)&size, INT64_SIZE);
                _blockSizes.push_back((std::streampos)size);
            }
            else
            {
                int size = 0;
                _in->read((char*)&size, INT_SIZE);
                if (_byteSwap) g_swapBytes((char*)&size, INT_SIZE);
                _blockSizes.push_back((std::streampos)size);
            }
        }
        else if (mark._name == "}" && !_beginPositions.empty())
        {
            _beginPositions.pop_back();
            _blockSizes.pop_back();
        }
    }
    checkStream();
}

void BinaryInputIterator::readCharArray(char* s, unsigned int size)
{
    if (size > 0) _in->read(s, size);
    checkStream();
}

void BinaryInputIterator::readWrappedString(std::string& str)
{
    readString(str);
}

void BinaryInputIterator::advanceToCurrentEndBracket()
{
    if (_supportBinaryBrackets && !_beginPositions.empty())
    {
        std::streampos position = _beginPositions.back();
        position += _blockSizes.back();
        _in->seekg(position);
        if (_in->fail()) _in->clear();
        _beginPositions.pop_back();
        _blockSizes.pop_back();
    }
    checkStream();
}

// ---------------------------------------------------------------------------
// InputStream
// ---------------------------------------------------------------------------
InputStream::InputStream() : _in(NULL), _exception(0), _arrayCapture(NULL)
{
    BEGIN_BRACKET.setName("{");
    BEGIN_BRACKET.setMark(+INDENT_VALUE);
    END_BRACKET.setName("}");
    END_BRACKET.setMark(-INDENT_VALUE);
}

InputStream::~InputStream() {}

ObjectProperty& InputStream::PROPERTY(const std::string& name, int value)
{
    _property.setName(name);
    _property.setValue(value);
    _property._mapProperty = false;
    return _property;
}

void InputStream::throwException(const std::string& msg)
{
    _exception = 1;
    if (_in) _in->throwException(msg);
}

void InputStream::setFileVersion(const std::string& domain, int version)
{
    _domainVersionMap[domain] = version;
}

int InputStream::getFileVersion(const std::string& domain) const
{
    if (domain.empty())
    {
        if (!_domainVersionMap.empty()) return _domainVersionMap.rbegin()->second;
        return 0;
    }
    DomainVersionMap::const_iterator itr = _domainVersionMap.find(domain);
    return itr != _domainVersionMap.end() ? itr->second : 0;
}

InputStream::ReadType InputStream::start(InputIterator* iterator)
{
    _in = iterator;
    if (getException()) return READ_UNKNOWN;

    _in->setInputStream(this);
    if (getException()) return READ_UNKNOWN;

    if (isBinary())
    {
        // Binary file header: [8-byte magic][typeValue][version][attributes][optional domains]
        // The magic has already been detected by readInputIterator; consume and validate it here.
        unsigned int headerLow = 0, headerHigh = 0;
        *this >> headerLow >> headerHigh;
        if (getException()) return READ_UNKNOWN;
        if (headerLow != OSG_HEADER_LOW || headerHigh != OSG_HEADER_HIGH)
        {
            std::cerr << "[InputStream] Bad binary header." << std::endl;
            return READ_UNKNOWN;
        }

        unsigned int typeValue = 0;
        unsigned int version = 0;
        unsigned int attributes = 0;

        *this >> typeValue >> version;
        if (getException()) return READ_UNKNOWN;

        *this >> attributes;
        if (getException()) return READ_UNKNOWN;

        if (attributes & 0x4) _in->setSupportBinaryBrackets(true);

        if (attributes & 0x1)
        {
            unsigned int numDomains = 0;
            *this >> numDomains;
            for (unsigned int i = 0; i < numDomains; ++i)
            {
                std::string domainName;
                int domainVersion;
                *this >> domainName >> domainVersion;
                if (getException()) return READ_UNKNOWN;
                setFileVersion(domainName, domainVersion);
            }
        }

        _domainVersionMap[""] = (int)version;
        return (ReadType)typeValue;
    }
    else
    {
        std::string firstString;
        *this >> firstString;
        if (getException()) return READ_UNKNOWN;

        if (firstString == "#Ascii" || firstString == "#Binary")
        {
            // Standard header: #Ascii Scene / #Ascii Image
            std::string typeString;
            *this >> typeString;
            if (getException()) return READ_UNKNOWN;

            // Read #Version <version>
            std::string versionToken;
            *this >> versionToken;
            int version = 0;
            if (versionToken == "#Version")
            {
                *this >> version;
                if (getException()) return READ_UNKNOWN;
                setFileVersion("osg", version);
            }

            // Optional #Generator name version
            while (matchString("#Generator"))
            {
                std::string genName, genVersion;
                *this >> genName >> genVersion;
                if (getException()) return READ_UNKNOWN;
            }

            // Optional #CustomDomain name version
            while (matchString("#CustomDomain"))
            {
                std::string domainName;
                int domainVersion;
                *this >> domainName >> domainVersion;
                if (getException()) return READ_UNKNOWN;
                setFileVersion(domainName, domainVersion);
            }

            return (typeString == "Image") ? READ_IMAGE : READ_SCENE;
        }
        else if (firstString == "osg")
        {
            // Legacy format: osg <version> { ... }
            int version = 0;
            *this >> version;
            if (getException()) return READ_UNKNOWN;
            setFileVersion("osg", version);
            return READ_SCENE;
        }
        else if (firstString == "image")
        {
            int version = 0;
            *this >> version;
            if (getException()) return READ_UNKNOWN;
            setFileVersion("", version);
            return READ_IMAGE;
        }
        return READ_UNKNOWN;
    }
}

void InputStream::decompress()
{
    if (!_in || !isBinary()) return;

    std::string compressorName;
    *this >> compressorName;
    if (getException()) return;

    if (compressorName == "0") return;

    // Only uncompressed binary is supported; zlib and other compression formats are not
    throwException("Unsupported compression: " + compressorName);
}

unsigned int InputStream::readSize()
{
    if (isBinary())
    {
        unsigned int value = 0;
        _in->readUInt(value);
        checkStream();
        return value;
    }
    else
    {
        unsigned int value = 0;
        *this >> value;
        return value;
    }
}

ObjectTypeAndID InputStream::readObject(InputUserData& ud)
{
    std::streampos startPos = tellg();
    std::string className;
    *this >> className;
    if (getException()) return ObjectTypeAndID();
    if (className == "NULL") return ObjectTypeAndID("", "");

    unsigned int id = 0;
    *this >> BEGIN_BRACKET >> PROPERTY("UniqueID") >> id;
    if (getException()) return ObjectTypeAndID();

    IdentifierMap::iterator itr = _identifierMap.find(id);
    if (itr != _identifierMap.end())
    {
        _in->advanceToCurrentEndBracket();
        return itr->second;
    }

    // Record a scene-tree node for diagnostics printing (see InputUserData::TreeNode).
    // The RAII guard restores the parent context on every return path below.
    int nodeIdx = ud.addTreeNode(className, std::to_string(id), startPos);
    ud.pushTreeNode(nodeIdx);
    struct TreeGuard
    {
        InputUserData* ud;
        ~TreeGuard() { ud->popTreeNode(); }
    } treeGuard = { &ud };

    ObjectTypeAndID oid = readObjectFields(className, std::to_string(id), ud);
    if (getException()) return ObjectTypeAndID();

    _identifierMap[id] = oid;
    _in->advanceToCurrentEndBracket();
    return oid;
}

namespace
{
    // Read the serializers of a single wrapper (only this wrapper's own fields, not its associates).
    // Equivalent to the legacy ObjectWrapper::read; expansion of associates is driven by readWrapperFields.
    bool readSerializerList(InputStream& is, InputUserData& ud,
                            Rewrapper* wrapper, int inputVersion)
    {
        const std::vector<Rewrapper::SerializerPair>& sl = wrapper->getSerializers();
        for (size_t i = 0; i < sl.size(); ++i)
        {
            BaseSerializer* s = sl[i].second.get();
            if (!s) continue;
            if (s->_firstVersion > inputVersion || inputVersion > s->_lastVersion) continue;

            if (!is.isBinary())
            {
                // In ASCII mode the property name matches the field one-to-one;
                // a mismatch means this field was not written to the file.
                if (!is.matchString(sl[i].first)) continue;
            }

            if (!readSerializerValue(is, ud, *s))
            {
                std::cerr << "[InputStream] Unhandled serializer '" << sl[i].first
                          << "' (" << typeid(*s).name() << ")\n";
            }
            else
            {
                // Deliver the just-read value (property name + serializer) to the
                // engine visitor. accept() pushes the property name so the
                // visitor's apply() can dispatch on it.
                SerializerVisitor* visitor = ud.getVisitor();
                if (visitor)
                {
                    visitor->setInput(&is, &ud);
                    s->accept(sl[i].first, *visitor);
                }
            }
            if (is.getException()) return false;
        }
        return true;
    }

    // Read the associated class fields and own fields in the writer order.
    // The last token in the associates list is the class itself
    // (e.g. "osg::Object osg::Node osg::Drawable osg::Geometry"),
    // so we only need to call readSerializerList for each entry of the list;
    // no recursion is needed -- recursion would loop forever on the self token.
    bool readWrapperFields(InputStream& is, InputUserData& ud,
                           Rewrapper* wrapper, int inputVersion)
    {
        const std::vector<WrapperAssociate>& assocs = wrapper->getAssociates();
        for (size_t i = 0; i < assocs.size(); ++i)
        {
            const WrapperAssociate& a = assocs[i];
            if (a._firstVersion <= inputVersion && inputVersion <= a._lastVersion)
            {
                Rewrapper* r = RewrapperManager::instance()->getRewrapper(a._name);
                if (r != NULL)
                {
                    if (!readSerializerList(is, ud, r, inputVersion)) return false;
                }
                else std::cerr << "[InputStream] Associated wrapper " << a._name << " not found\n";
            }
        }
        return true;
    }
}

ObjectTypeAndID InputStream::readObjectFields(const std::string& className, const std::string& id, InputUserData& ud)
{
    RewrapperManager* manager = RewrapperManager::instance().get();
    Rewrapper* wrapper = manager ? manager->getRewrapper(className) : NULL;
    if (!wrapper)
    {
        std::cerr << "[InputStream] No rewrapper registered for "
                  << className.substr(0, 40) << ", skipping fields." << std::endl;
        return ObjectTypeAndID(className, id);
    }

    if (!readWrapperFields(*this, ud, wrapper, getFileVersion()))
        return ObjectTypeAndID();

    return ObjectTypeAndID(className, id);
}

void InputStream::readArrayImplementation(unsigned int arrayType, unsigned int numComponents,
                                          unsigned int componentSizeInBytes,
                                          std::vector<double>* out)
{
    if (!out) out = _arrayCapture;
    unsigned int size = 0;
    *this >> size >> BEGIN_BRACKET;
    if (getException()) return;

    if (size > 0)
    {
        bool isDoubleType =
            (arrayType == ID_DOUBLE_ARRAY || arrayType == ID_VEC2D_ARRAY ||
             arrayType == ID_VEC3D_ARRAY || arrayType == ID_VEC4D_ARRAY);
        bool isFloatType =
            (arrayType == ID_FLOAT_ARRAY || arrayType == ID_VEC2_ARRAY ||
             arrayType == ID_VEC3_ARRAY || arrayType == ID_VEC4_ARRAY);
        bool isInt64Type = (arrayType == ID_INT64_ARRAY || arrayType == ID_UINT64_ARRAY);

        if (isBinary())
        {
            if (out)
            {
                // Capture path (an engine visitor / IR recording is active):
                // read element by element and materialize component doubles.
                unsigned int bytesPerElement = numComponents * componentSizeInBytes;
                std::vector<char> row(bytesPerElement ? bytesPerElement : 1);
                for (unsigned int i = 0; i < size; ++i)
                {
                    if (bytesPerElement)
                        readComponentArray(&row[0], 1, numComponents, componentSizeInBytes);
                    for (unsigned int j = 0; j < numComponents; ++j)
                    {
                        const char* p = &row[j * componentSizeInBytes];
                        double value = 0.0;
                        if (isInt64Type) { long long t = 0; std::memcpy(&t, p, 8); value = (double)t; }
                        else if (isDoubleType) { double t = 0; std::memcpy(&t, p, 8); value = t; }
                        else if (isFloatType) { float t = 0; std::memcpy(&t, p, 4); value = (double)t; }
                        else
                        {
                            unsigned long long raw = 0;
                            std::memcpy(&raw, p, componentSizeInBytes);
                            value = (double)(long long)raw;
                        }
                        out->push_back(value);
                    }
                }
            }
            else
            {
                unsigned int bytesPerElement = numComponents * componentSizeInBytes;
                unsigned int elementsPerChunk = 4096;
                if (bytesPerElement > 0) elementsPerChunk = 4096 / bytesPerElement;
                if (elementsPerChunk == 0) elementsPerChunk = 1;

                std::vector<char> buffer;
                unsigned int remaining = size;
                while (remaining > 0)
                {
                    unsigned int n = (remaining < elementsPerChunk) ? remaining : elementsPerChunk;
                    buffer.resize(n * bytesPerElement);
                    if (!buffer.empty())
                        readComponentArray(&buffer[0], n, numComponents, componentSizeInBytes);
                    remaining -= n;
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < size; ++i)
            {
                for (unsigned int j = 0; j < numComponents; ++j)
                {
                    if (isInt64Type) { long long v; *this >> v; if (out) out->push_back((double)v); }
                    else if (isDoubleType) { double v; *this >> v; if (out) out->push_back(v); }
                    else if (isFloatType) { float v; *this >> v; if (out) out->push_back((double)v); }
                    else { int v; *this >> v; if (out) out->push_back((double)v); }
                }
            }
        }
    }
    *this >> END_BRACKET;
}

ObjectTypeAndID InputStream::readArray(InputUserData& ud)
{
    unsigned int id = 0;
    *this >> PROPERTY("ArrayID") >> id;
    if (getException()) return ObjectTypeAndID();

    ArrayMap::iterator itr = _arrayMap.find(id);
    if (itr != _arrayMap.end())
    {
        _in->advanceToCurrentEndBracket();
        return itr->second;
    }

    DEF_MAPPEE(ArrayType, type);
    *this >> type;
    if (getException()) return ObjectTypeAndID();

    IntLookup& arrayTable = UserLookupTableProxy::get("ArrayType");
    std::string typeName = arrayTable.value(type.get());
    if (typeName.empty()) typeName = type._name;
    if (typeName.empty() || typeName == "ArrayType") typeName = "Array";

    // Legacy compact arrays carry no class name: expose the mapped osg class and
    // the element data to the visitor / IR.
    bool record = (ud.getVisitor() != NULL);
    std::vector<double> values;
    if (record)
    {
        _arrayCapture = &values;
        ud.notifyBeginObject(g_osgArrayClass(type.get()), std::to_string(id));
    }

    switch (type.get())
    {
    case ID_BYTE_ARRAY:   readArrayImplementation(ID_BYTE_ARRAY, 1, CHAR_SIZE); break;
    case ID_UBYTE_ARRAY:  readArrayImplementation(ID_UBYTE_ARRAY, 1, CHAR_SIZE); break;
    case ID_SHORT_ARRAY:  readArrayImplementation(ID_SHORT_ARRAY, 1, SHORT_SIZE); break;
    case ID_USHORT_ARRAY: readArrayImplementation(ID_USHORT_ARRAY, 1, SHORT_SIZE); break;
    case ID_INT_ARRAY:    readArrayImplementation(ID_INT_ARRAY, 1, INT_SIZE); break;
    case ID_UINT_ARRAY:   readArrayImplementation(ID_UINT_ARRAY, 1, INT_SIZE); break;
    case ID_FLOAT_ARRAY:  readArrayImplementation(ID_FLOAT_ARRAY, 1, FLOAT_SIZE); break;
    case ID_DOUBLE_ARRAY: readArrayImplementation(ID_DOUBLE_ARRAY, 1, DOUBLE_SIZE); break;
    case ID_VEC2B_ARRAY:  readArrayImplementation(ID_VEC2B_ARRAY, 2, CHAR_SIZE); break;
    case ID_VEC3B_ARRAY:  readArrayImplementation(ID_VEC3B_ARRAY, 3, CHAR_SIZE); break;
    case ID_VEC4B_ARRAY:  readArrayImplementation(ID_VEC4B_ARRAY, 4, CHAR_SIZE); break;
    case ID_VEC4UB_ARRAY: readArrayImplementation(ID_VEC4UB_ARRAY, 4, CHAR_SIZE); break;
    case ID_VEC2S_ARRAY:  readArrayImplementation(ID_VEC2S_ARRAY, 2, SHORT_SIZE); break;
    case ID_VEC3S_ARRAY:  readArrayImplementation(ID_VEC3S_ARRAY, 3, SHORT_SIZE); break;
    case ID_VEC4S_ARRAY:  readArrayImplementation(ID_VEC4S_ARRAY, 4, SHORT_SIZE); break;
    case ID_VEC2_ARRAY:   readArrayImplementation(ID_VEC2_ARRAY, 2, FLOAT_SIZE); break;
    case ID_VEC3_ARRAY:   readArrayImplementation(ID_VEC3_ARRAY, 3, FLOAT_SIZE); break;
    case ID_VEC4_ARRAY:   readArrayImplementation(ID_VEC4_ARRAY, 4, FLOAT_SIZE); break;
    case ID_VEC2D_ARRAY:  readArrayImplementation(ID_VEC2D_ARRAY, 2, DOUBLE_SIZE); break;
    case ID_VEC3D_ARRAY:  readArrayImplementation(ID_VEC3D_ARRAY, 3, DOUBLE_SIZE); break;
    case ID_VEC4D_ARRAY:  readArrayImplementation(ID_VEC4D_ARRAY, 4, DOUBLE_SIZE); break;
    case ID_VEC2UB_ARRAY: readArrayImplementation(ID_VEC2UB_ARRAY, 2, CHAR_SIZE); break;
    case ID_VEC3UB_ARRAY: readArrayImplementation(ID_VEC3UB_ARRAY, 3, CHAR_SIZE); break;
    case ID_VEC2US_ARRAY: readArrayImplementation(ID_VEC2US_ARRAY, 2, SHORT_SIZE); break;
    case ID_VEC3US_ARRAY: readArrayImplementation(ID_VEC3US_ARRAY, 3, SHORT_SIZE); break;
    case ID_VEC4US_ARRAY: readArrayImplementation(ID_VEC4US_ARRAY, 4, SHORT_SIZE); break;
    case ID_VEC2I_ARRAY:  readArrayImplementation(ID_VEC2I_ARRAY, 2, INT_SIZE); break;
    case ID_VEC3I_ARRAY:  readArrayImplementation(ID_VEC3I_ARRAY, 3, INT_SIZE); break;
    case ID_VEC4I_ARRAY:  readArrayImplementation(ID_VEC4I_ARRAY, 4, INT_SIZE); break;
    case ID_VEC2UI_ARRAY: readArrayImplementation(ID_VEC2UI_ARRAY, 2, INT_SIZE); break;
    case ID_VEC3UI_ARRAY: readArrayImplementation(ID_VEC3UI_ARRAY, 3, INT_SIZE); break;
    case ID_VEC4UI_ARRAY: readArrayImplementation(ID_VEC4UI_ARRAY, 4, INT_SIZE); break;
    case ID_UINT64_ARRAY: readArrayImplementation(ID_UINT64_ARRAY, 1, INT64_SIZE); break;
    case ID_INT64_ARRAY:  readArrayImplementation(ID_INT64_ARRAY, 1, INT64_SIZE); break;
    default:
        _in->advanceToCurrentEndBracket();
        break;
    }
    _arrayCapture = NULL;
    if (getException())
    {
        if (record) ud.notifyEndObject();
        return ObjectTypeAndID();
    }

    if (record)
    {
        ud.notifyArrayData(g_arrayComponents(type.get()), values);
        ud.notifyEndObject();
    }

    ObjectTypeAndID oid(typeName, std::to_string(id));
    _arrayMap[id] = oid;
    return oid;
}

ObjectTypeAndID InputStream::readPrimitiveSet(InputUserData& ud)
{
    // Legacy compact primitive sets have no UniqueID in the stream; use the file
    // offset as a synthetic, per-file-unique id so references can resolve.
    long long primId = (long long)tellg();

    DEF_MAPPEE(PrimitiveType, type);
    DEF_MAPPEE(PrimitiveType, mode);
    *this >> type >> mode;
    if (getException()) return ObjectTypeAndID();

    unsigned int numInstances = 1;
    if (getFileVersion() > 96) *this >> numInstances;
    if (getException()) return ObjectTypeAndID();

    IntLookup& primTable = UserLookupTableProxy::get("PrimitiveType");
    std::string typeName = primTable.value(type.get());
    if (typeName.empty()) typeName = type._name;
    if (typeName.empty() || typeName == "PrimitiveType") typeName = "PrimitiveSet";

    const char* osgClass = "osg::PrimitiveSet";
    switch (type.get())
    {
    case ID_DRAWARRAYS:          osgClass = "osg::DrawArrays"; break;
    case ID_DRAWARRAY_LENGTH:    osgClass = "osg::DrawArrayLengths"; break;
    case ID_DRAWELEMENTS_UBYTE:  osgClass = "osg::DrawElementsUByte"; break;
    case ID_DRAWELEMENTS_USHORT: osgClass = "osg::DrawElementsUShort"; break;
    case ID_DRAWELEMENTS_UINT:   osgClass = "osg::DrawElementsUInt"; break;
    default: break;
    }

    bool record = (ud.getVisitor() != NULL);
    if (record) ud.notifyBeginObject(osgClass, std::to_string(primId));

    unsigned int first = 0, count = 0;
    std::vector<unsigned int> indices, lengths;
    switch (type.get())
    {
    case ID_DRAWARRAYS:
    {
        int f = 0, c = 0; *this >> f >> c;
        first = (unsigned int)f; count = (unsigned int)c;
        break;
    }
    case ID_DRAWARRAY_LENGTH:
    {
        // Layout (see osgDB::OutputStream::writePrimitiveSet): mode, first, size, {lengths}
        int f = 0; *this >> f;
        first = (unsigned int)f;

        unsigned int numPrimitives = 0;
        *this >> numPrimitives >> BEGIN_BRACKET;
        for (unsigned int i = 0; i < numPrimitives; ++i)
        {
            unsigned int v = 0; *this >> v;
            if (record) lengths.push_back(v);
            count += v;  // total vertex count of the concatenated length runs
        }
        *this >> END_BRACKET;
        break;
    }
    case ID_DRAWELEMENTS_UBYTE:
    case ID_DRAWELEMENTS_USHORT:
    case ID_DRAWELEMENTS_UINT:
    {
        unsigned int numIndices = 0;
        *this >> numIndices >> BEGIN_BRACKET;
        count = numIndices;
        if (isBinary())
        {
            unsigned int compSize = (type.get() == ID_DRAWELEMENTS_UBYTE) ? CHAR_SIZE :
                                    (type.get() == ID_DRAWELEMENTS_USHORT) ? SHORT_SIZE : INT_SIZE;
            std::vector<char> buffer;
            buffer.resize(numIndices * compSize);
            if (!buffer.empty()) readComponentArray(&buffer[0], numIndices, 1, compSize);
            if (record)
            {
                for (unsigned int i = 0; i < numIndices; ++i)
                {
                    unsigned long long raw = 0;
                    std::memcpy(&raw, &buffer[i * compSize], compSize);
                    indices.push_back((unsigned int)raw);
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < numIndices; ++i)
            { unsigned int v; *this >> v; if (record) indices.push_back(v); }
        }
        *this >> END_BRACKET;
        break;
    }
    default:
        break;
    }
    if (getException())
    {
        if (record) ud.notifyEndObject();
        return ObjectTypeAndID();
    }

    if (record)
    {
        ud.notifyPrimitiveData(mode.get(), first, count, indices, lengths);
        ud.notifyEndObject();
    }

    (void)numInstances;
    return ObjectTypeAndID(typeName, std::to_string(primId));
}

ObjectTypeAndID InputStream::readImage(InputUserData& ud)
{
    if (getFileVersion() > 94)
    {
        std::string className;
        *this >> PROPERTY("ClassName") >> className;
        if (getException()) return ObjectTypeAndID();
    }

    unsigned int id = 0;
    *this >> PROPERTY("UniqueID") >> id;
    if (getException()) return ObjectTypeAndID();

    ImageMap::iterator itr = _imageMap.find(id);
    if (itr != _imageMap.end())
    {
        // In the standard osg format the Image has no BEGIN/END bracket of its own
        // (writeImage writes only className + UniqueID, plus
        // FileName/WriteHint/decision/data for new images).
        // Therefore at the shared-reference site we must not call
        // advanceToCurrentEndBracket(); doing so would wrongly pop and seek
        // to the end of the host object (e.g. Texture2D) block.
        // See osgDB's InputStream::readImage(): it returns directly after a
        // hit in _identifierMap.
        return itr->second;
    }

    std::string name;
    *this >> PROPERTY("FileName");
    readWrappedString(name);
    if (getException()) return ObjectTypeAndID();

    int writeHint = 0, decision = 0;
    *this >> PROPERTY("WriteHint") >> writeHint >> decision;
    if (getException()) return ObjectTypeAndID();

    // Capture the image payload for the visitor / IR: the file name for
    // external-image references, or the pixel data for inline images.
    bool record = (ud.getVisitor() != NULL);
    if (record) ud.notifyBeginObject("osg::Image", std::to_string(id));

    std::string fileName = name;
    int origin = 0, imgS = 0, imgT = 0, imgR = 1, internalFormat = 0;
    int pixelFormat = 0, dataType = 0, packing = 0, allocationMode = 0;
    std::vector<unsigned char> pixels;

    if (decision == IMAGE_INLINE_DATA)
    {
        if (isBinary())
        {
            // Standard osg binary format:
            //   origin, s, t, r, internalFormat     (5 x int)
            //   pixelFormat, dataType, packing, mode (4 x int)
            //   size + image data
            //   levelSize + levels[i] (per-mipmap data sizes)
            int s, t, r, internalFormatTmp;
            *this >> origin >> s >> t >> r >> internalFormatTmp;
            internalFormat = internalFormatTmp;
            int pixelFormatTmp, dataTypeTmp, packingTmp, mode;
            *this >> pixelFormatTmp >> dataTypeTmp >> packingTmp >> mode;
            imgS = s; imgT = t; imgR = (r > 0 ? r : 1);
            pixelFormat = pixelFormatTmp; dataType = dataTypeTmp;
            packing = packingTmp; allocationMode = mode;
            if (getException()) { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }

            unsigned int size = 0;
            *this >> size;
            if (size > 0)
            {
                pixels.resize(size);
                _in->readCharArray((char*)&pixels[0], size);
            }
            if (getException()) { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }

            unsigned int levelSize = 0;
            *this >> levelSize;
            for (unsigned int i = 0; i < levelSize; ++i)
            {
                unsigned int level = 0;
                *this >> level;
                if (getException()) { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }
            }
        }
        else
        {
            int s, t, r, mode;
            *this >> PROPERTY("Origin") >> origin;
            *this >> PROPERTY("Size") >> s >> t >> r;
            *this >> PROPERTY("InternalTextureFormat") >> internalFormat;
            *this >> PROPERTY("PixelFormat") >> pixelFormat;
            *this >> PROPERTY("DataType") >> dataType;
            *this >> PROPERTY("Packing") >> packing;
            *this >> PROPERTY("AllocationMode") >> mode;
            imgS = s; imgT = t; imgR = (r > 0 ? r : 1); allocationMode = mode;
            if (getException()) { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }

            *this >> PROPERTY("Data");
            unsigned int levelSize = readSize();
            if (levelSize > 0) --levelSize;
            *this >> BEGIN_BRACKET;
            for (unsigned int i = 0; i < levelSize + 1; ++i)
            {
                std::string base64Data;
                readWrappedString(base64Data);
                if (getException()) { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }
            }
            *this >> END_BRACKET;
        }
    }
    else if (decision == IMAGE_INLINE_FILE)
    {
        if (isBinary())
        {
            // Standard binary: size + embedded file data
            unsigned int size = 0;
            *this >> size;
            if (size > 0)
            {
                pixels.resize(size);
                _in->readCharArray((char*)&pixels[0], size);
            }
            if (getException()) { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }
        }
    }
    if (getException()) { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }

    // The writer (OutputStream::writeImage) always follows the image payload with
    // the image's own osg::Object fields - Name / DataVariance / UserData(UserDataContainer)
    // (writeObjectFields(img, "osg::Object") in OutputStream.cpp). osgDB's InputStream::readImage()
    // mirrors this with readObjectFieldsOfType<osg::Image>("osg::Object", id, ...), so we must
    // consume those bytes too, otherwise every following record is misaligned.
    {
        ObjectTypeAndID objectFields = readObjectFields("osg::Object", std::to_string(id), ud);
        if (getException() || !objectFields.valid())
        { if (record) ud.notifyEndObject(); return ObjectTypeAndID(); }
    }

    if (record)
    {
        ud.notifyImageData(fileName, origin, imgS, imgT, imgR,
                           internalFormat, pixelFormat, dataType,
                           packing, allocationMode, decision, pixels);
        ud.notifyEndObject();
    }

    ObjectTypeAndID oid("osg::Image", std::to_string(id));
    _imageMap[id] = oid;
    return oid;
}

ObjectTypeAndID InputStream::readObjectFromStream(InputUserData& ud, const std::string& type)
{
    return ud.readObjectFromStream(*this, type);
}

// ---- operator>> basic types ----
InputStream& InputStream::operator>>(bool& b)
{ _in->readBool(b); checkStream(); return *this; }
InputStream& InputStream::operator>>(char& c)
{ _in->readChar(c); checkStream(); return *this; }
InputStream& InputStream::operator>>(signed char& c)
{ _in->readSChar(c); checkStream(); return *this; }
InputStream& InputStream::operator>>(unsigned char& c)
{ _in->readUChar(c); checkStream(); return *this; }
InputStream& InputStream::operator>>(short& s)
{ _in->readShort(s); checkStream(); return *this; }
InputStream& InputStream::operator>>(unsigned short& s)
{ _in->readUShort(s); checkStream(); return *this; }
InputStream& InputStream::operator>>(int& i)
{ _in->readInt(i); checkStream(); return *this; }
InputStream& InputStream::operator>>(unsigned int& i)
{ _in->readUInt(i); checkStream(); return *this; }
InputStream& InputStream::operator>>(long& l)
{ _in->readLong(l); checkStream(); return *this; }
InputStream& InputStream::operator>>(unsigned long& l)
{ _in->readULong(l); checkStream(); return *this; }
InputStream& InputStream::operator>>(long long& l)
{
    if (isBinary())
    {
        char buf[8];
        _in->readCharArray(buf, 8);
        if (_in->getByteSwap()) g_swapBytes(buf, 8);
        std::memcpy(&l, buf, 8);
        checkStream();
    }
    else
    {
        std::string str;
        *this >> str;
        l = (long long)strtoll(str.c_str(), NULL, 0);
    }
    return *this;
}
InputStream& InputStream::operator>>(unsigned long long& l)
{
    if (isBinary())
    {
        char buf[8];
        _in->readCharArray(buf, 8);
        if (_in->getByteSwap()) g_swapBytes(buf, 8);
        std::memcpy(&l, buf, 8);
        checkStream();
    }
    else
    {
        std::string str;
        *this >> str;
        l = (unsigned long long)strtoull(str.c_str(), NULL, 0);
    }
    return *this;
}
InputStream& InputStream::operator>>(float& f)
{ _in->readFloat(f); checkStream(); return *this; }
InputStream& InputStream::operator>>(double& d)
{ _in->readDouble(d); checkStream(); return *this; }
InputStream& InputStream::operator>>(std::string& str)
{ _in->readString(str); checkStream(); return *this; }
InputStream& InputStream::operator>>(InputStream& (*fn)(InputStream&))
{ if (fn) fn(*this); return *this; }
InputStream& InputStream::operator>>(std::ios_base& (*fn)(std::ios_base&))
{ _in->readBase(fn); checkStream(); return *this; }
InputStream& InputStream::operator>>(ObjectGLenum& v)
{ _in->readGLenum(v); checkStream(); return *this; }
InputStream& InputStream::operator>>(ObjectProperty& p)
{ _in->readProperty(p); checkStream(); return *this; }
InputStream& InputStream::operator>>(ObjectMark& m)
{ _in->readMark(m); checkStream(); return *this; }

// ---- operator>> math types ----
InputStream& InputStream::operator>>(osg::Quat& q)
{
    double x, y, z, w;
    *this >> x >> y >> z >> w;
    q._v[0] = x; q._v[1] = y; q._v[2] = z; q._v[3] = w;
    return *this;
}

InputStream& InputStream::operator>>(osg::Plane& p)
{
    *this >> p._v[0] >> p._v[1] >> p._v[2] >> p._v[3];
    return *this;
}

InputStream& InputStream::operator>>(osg::Matrixd& m)
{
    *this >> BEGIN_BRACKET;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
        {
            double v; *this >> v; m._m[c * 4 + r] = v;
        }
    *this >> END_BRACKET;
    return *this;
}

InputStream& InputStream::operator>>(osg::Matrixf& m)
{
    *this >> BEGIN_BRACKET;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
        {
            double v; *this >> v; m._m[c * 4 + r] = (float)v;
        }
    *this >> END_BRACKET;
    return *this;
}

InputStream& InputStream::operator>>(osg::BoundingBoxf& bb)
{
    float p0, p1, p2, p3, p4, p5;
    *this >> p0 >> p1 >> p2 >> p3 >> p4 >> p5;
    bb._min._v[0] = p0; bb._min._v[1] = p1; bb._min._v[2] = p2;
    bb._max._v[0] = p3; bb._max._v[1] = p4; bb._max._v[2] = p5;
    return *this;
}

InputStream& InputStream::operator>>(osg::BoundingBoxd& bb)
{
    double p0, p1, p2, p3, p4, p5;
    *this >> p0 >> p1 >> p2 >> p3 >> p4 >> p5;
    bb._min._v[0] = p0; bb._min._v[1] = p1; bb._min._v[2] = p2;
    bb._max._v[0] = p3; bb._max._v[1] = p4; bb._max._v[2] = p5;
    return *this;
}

InputStream& InputStream::operator>>(osg::BoundingSpheref& bs)
{
    float p0, p1, p2, p3;
    *this >> p0 >> p1 >> p2 >> p3;
    bs._center._v[0] = p0; bs._center._v[1] = p1; bs._center._v[2] = p2;
    bs._radius = p3;
    return *this;
}

InputStream& InputStream::operator>>(osg::BoundingSphered& bs)
{
    double p0, p1, p2, p3;
    *this >> p0 >> p1 >> p2 >> p3;
    bs._center._v[0] = p0; bs._center._v[1] = p1; bs._center._v[2] = p2;
    bs._radius = p3;
    return *this;
}

// ---------------------------------------------------------------------------
// InputUserData
// ---------------------------------------------------------------------------
InputUserData::InputUserData()
    : _visitor(NULL), _ir(NULL)
{
}

InputUserData::~InputUserData() {}

void InputUserData::clear()
{
    _objectMap.clear();
    _treeNodes.clear();
    _treeStack.clear();
    _irStack.clear();
    _treeToIr.clear();
}

void InputUserData::notifyBeginObject(const std::string& className, const std::string& id)
{
    if (_visitor) _visitor->beginObject(className, id);
}

void InputUserData::notifyEndObject()
{
    if (_visitor) _visitor->endObject();
}

void InputUserData::notifyArrayData(int components, const std::vector<double>& values)
{
    if (_visitor) _visitor->arrayData(components, values);
}

void InputUserData::notifyPrimitiveData(int mode, unsigned int first, unsigned int count,
                                        const std::vector<unsigned int>& indices,
                                        const std::vector<unsigned int>& lengths)
{
    if (_visitor) _visitor->primitiveData(mode, first, count, indices, lengths);
}

void InputUserData::notifyImageData(const std::string& fileName, int origin,
                                    int width, int height, int depth,
                                    int internalFormat, int pixelFormat, int dataType,
                                    int packing, int allocationMode, int decision,
                                    const std::vector<unsigned char>& pixels)
{
    if (_visitor) _visitor->imageData(fileName, origin, width, height, depth,
                                      internalFormat, pixelFormat, dataType,
                                      packing, allocationMode, decision, pixels);
}

void InputUserData::emitCommand(const char* name, const std::vector<IRArg>& args)
{
    if (_ir) _ir->emitCommand(currentIR(), name ? name : "", args);
    if (_visitor) _visitor->command(name ? name : "", args);
}

void InputUserData::emitCommand(const ObjectTypeAndID& parent, const char* name,
                                const std::vector<IRArg>& args)
{
    // Explicit-target commands (e.g. an Array's setBinding) are recorded against
    // the addressed object in the IR. They are not forwarded to the visitor,
    // which only tracks the current object scope (v>=112 files carry array
    // binding/normalize through the Array object's own serializers instead).
    if (_ir)
    {
        int target = _ir->resolve(parent._type, parent._id);
        if (target < 0) target = currentIR();
        _ir->emitCommand(target, name ? name : "", args);
    }
}

ObjectTypeAndID InputUserData::readObjectFromStream(InputStream& is, const std::string& type)
{
    ObjectTypeAndID oid;
    if (is.isBinary())
    {
        if (is.getFileVersion() >= 112)
        {
            // osgVerse custom variant (fileVersion >= 112): after the bool flag
            // the object content follows directly (className + BEGIN_BRACKET + ...),
            // with no extra brackets. Array objects are written in the full-object
            // form just like regular objects (className + UniqueID + fields).
            bool hasObject = false;
            is >> hasObject;
            if (!hasObject || is.getException()) return ObjectTypeAndID();
            if (type == "osg::PrimitiveSet") oid = is.readPrimitiveSet(*this);
            else oid = is.readObject(*this);
            return oid;
        }

        // Legacy standard format (fileVersion < 112): the embedded reference follows
        // directly after the caller's own bool flags - no extra flag here. Arrays use
        // the compact readArray form (ArrayID + ArrayType + data), primitive sets use
        // the compact readPrimitiveSet form (type + mode + data), e.g. VertexData:
        //   [UserSerializer ok:1B][hasArray:1B][ArrayID:4B][ArrayType:4B][size][data]
        if (type.find("Array") != std::string::npos)
            oid = is.readArray(*this);
        else if (type == "osg::PrimitiveSet") oid = is.readPrimitiveSet(*this);
        else oid = is.readObject(*this);
        return oid;
    }

    // ASCII: the property name has already been consumed by matchString. Arrays
    // and primitive sets are self-delimiting (they read their own ArrayID/size
    // braces); other objects are a plain className + fields, with any wrapping
    // braces / has-object flag handled by the caller (user serializer or
    // ObjectSerializer). This keeps both call conventions consistent.
    if (type == "osg::Array") return is.readArray(*this);
    if (type == "osg::PrimitiveSet") return is.readPrimitiveSet(*this);
    return is.readObject(*this);
}
