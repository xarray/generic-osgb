#include <GenericReserializer.h>
using namespace osgVerse;

#define WRAPUNIFORMTEMAPLATE( TYPE, INHERITANCE_STRING, SERIALIZER_TYPE, DEFAULT) \
    namespace Wrap##TYPE \
    { \
        REGISTER_OBJECT_WRAPPER( TYPE, \
                                new osg::TYPE, \
                                osg::TYPE, \
                                INHERITANCE_STRING ) \
        { \
            SERIALIZER_TYPE( Value, DEFAULT ); \
        } \
    }


WRAPUNIFORMTEMAPLATE(IntUniform, "osg::Object osg::IntUniform", ADD_INT_SERIALIZER, 0)
WRAPUNIFORMTEMAPLATE(UIntUniform, "osg::Object osg::UIntUniform", ADD_UINT_SERIALIZER, 0u)
WRAPUNIFORMTEMAPLATE(FloatUniform, "osg::Object osg::FloatUniform", ADD_FLOAT_SERIALIZER, 0.0f)

WRAPUNIFORMTEMAPLATE(Vec2Uniform, "osg::Object osg::Vec2Uniform", ADD_VEC2F_SERIALIZER, osg::Vec2f())
WRAPUNIFORMTEMAPLATE(Vec3Uniform, "osg::Object osg::Vec3Uniform", ADD_VEC3F_SERIALIZER, osg::Vec3f())
WRAPUNIFORMTEMAPLATE(Vec4Uniform, "osg::Object osg::Vec4Uniform", ADD_VEC4F_SERIALIZER, osg::Vec4f())

WRAPUNIFORMTEMAPLATE(MatrixfUniform, "osg::Object osg::MatrixfUniform", ADD_MATRIXF_SERIALIZER, osg::Matrixf())
WRAPUNIFORMTEMAPLATE(MatrixdUniform, "osg::Object osg::MatrixdUniform", ADD_MATRIXD_SERIALIZER, osg::Matrixd())

#define WRAPUNIFORMARRAYTEMAPLATE( TYPE, INHERITANCE_STRING, ELEMENT_TYPE) \
    namespace Wrap##TYPE \
    { \
        REGISTER_OBJECT_WRAPPER( TYPE, \
                                new osg::TYPE, \
                                osg::TYPE, \
                                INHERITANCE_STRING ) \
        { \
            ADD_VECTOR_SERIALIZER(Array, osg::TYPE::array_type, ELEMENT_TYPE, 1); \
        } \
    }


WRAPUNIFORMARRAYTEMAPLATE(IntArrayUniform, "osg::Object osg::IntArrayUniform", osgDB::BaseSerializer::RW_INT)
WRAPUNIFORMARRAYTEMAPLATE(UIntArrayUniform, "osg::Object osg::UIntArrayUniform", osgDB::BaseSerializer::RW_UINT)
WRAPUNIFORMARRAYTEMAPLATE(FloatArrayUniform, "osg::Object osg::FloatArrayUniform", osgDB::BaseSerializer::RW_FLOAT)
WRAPUNIFORMARRAYTEMAPLATE(DoubleArrayUniform, "osg::Object osg::DoubleArrayUniform", osgDB::BaseSerializer::RW_DOUBLE)
WRAPUNIFORMARRAYTEMAPLATE(Vec2ArrayUniform, "osg::Object osg::Vec2ArrayUniform", osgDB::BaseSerializer::RW_VEC2F)
WRAPUNIFORMARRAYTEMAPLATE(Vec3ArrayUniform, "osg::Object osg::Vec3ArrayUniform", osgDB::BaseSerializer::RW_VEC3F)
WRAPUNIFORMARRAYTEMAPLATE(Vec4ArrayUniform, "osg::Object osg::Vec4ArrayUniform", osgDB::BaseSerializer::RW_VEC4F)
WRAPUNIFORMARRAYTEMAPLATE(Vec2iArrayUniform, "osg::Object osg::Vec2iArrayUniform", osgDB::BaseSerializer::RW_VEC2I)
WRAPUNIFORMARRAYTEMAPLATE(Vec3iArrayUniform, "osg::Object osg::Vec3iArrayUniform", osgDB::BaseSerializer::RW_VEC3I)
WRAPUNIFORMARRAYTEMAPLATE(Vec4iArrayUniform, "osg::Object osg::Vec4iArrayUniform", osgDB::BaseSerializer::RW_VEC4I)
WRAPUNIFORMARRAYTEMAPLATE(Vec2uiArrayUniform, "osg::Object osg::Vec2uiArrayUniform", osgDB::BaseSerializer::RW_VEC2UI)
WRAPUNIFORMARRAYTEMAPLATE(Vec3uiArrayUniform, "osg::Object osg::Vec3uiArrayUniform", osgDB::BaseSerializer::RW_VEC3UI)
WRAPUNIFORMARRAYTEMAPLATE(Vec4uiArrayUniform, "osg::Object osg::Vec4uiArrayUniform", osgDB::BaseSerializer::RW_VEC4UI)
WRAPUNIFORMARRAYTEMAPLATE(MatrixfArrayUniform, "osg::Object osg::MatrixfArrayUniform", osgDB::BaseSerializer::RW_MATRIXF)
WRAPUNIFORMARRAYTEMAPLATE(MatrixdArrayUniform, "osg::Object osg::MatrixdArrayUniform", osgDB::BaseSerializer::RW_MATRIXD)


//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Old osg::Uniform serializer
static bool readElements(InputStream& is, InputUserData& ud)
{
    bool hasArray; is >> hasArray;
    if ( hasArray )
    {
        ObjectTypeAndID array = ud.readObjectFromStream(is, "osg::Array");
        if ( array.valid() ) ud.add("setArray", &array);
    }
    return true;
}

static bool hasElements(OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setArray");
    return cmd && !cmd->args.empty() && ud.resolve(cmd->args[0]) >= 0;
}

static bool writeElements(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setArray");
    int target = (cmd && !cmd->args.empty()) ? ud.resolve(cmd->args[0]) : -1;

    // Binary layout expected by readElements(): [hasArray][hasObject][object]
    // ("hasObject" belongs to readObjectFromStream()); ASCII only needs the
    // first flag before the object.
    os << (target >= 0);
    if (os.isBinary()) os << (target >= 0);
    if (target >= 0) os.writeObject(ud, target);
    if (!os.isBinary()) os << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Uniform,
                         new osg::Uniform,
                         osg::Uniform,
                         "osg::Object osg::Uniform" )
{
    BEGIN_ENUM_SERIALIZER( Type, UNDEFINED );
        ADD_ENUM_VALUE2( FLOAT, 0x1406 );
        ADD_ENUM_VALUE2( FLOAT_VEC2, 0x8B50 );
        ADD_ENUM_VALUE2( FLOAT_VEC3, 0x8B51 );
        ADD_ENUM_VALUE2( FLOAT_VEC4, 0x8B52 );

        ADD_ENUM_VALUE2( DOUBLE, 0x140A );
        ADD_ENUM_VALUE2( DOUBLE_VEC2, 0x8FFC );
        ADD_ENUM_VALUE2( DOUBLE_VEC3, 0x8FFD );
        ADD_ENUM_VALUE2( DOUBLE_VEC4, 0x8FFE );

        ADD_ENUM_VALUE2( INT, 0x1404 );
        ADD_ENUM_VALUE2( INT_VEC2, 0x8B53 );
        ADD_ENUM_VALUE2( INT_VEC3, 0x8B54 );
        ADD_ENUM_VALUE2( INT_VEC4, 0x8B55 );

        ADD_ENUM_VALUE2( UNSIGNED_INT, 0x1405 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_VEC2, 0x8DC6 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_VEC3, 0x8DC7 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_VEC4, 0x8DC8 );

        ADD_ENUM_VALUE2( BOOL, 0x8B56 );
        ADD_ENUM_VALUE2( BOOL_VEC2, 0x8B57 );
        ADD_ENUM_VALUE2( BOOL_VEC3, 0x8B58 );
        ADD_ENUM_VALUE2( BOOL_VEC4, 0x8B59 );

        ADD_ENUM_VALUE2( FLOAT_MAT2, 0x8B5A );
        ADD_ENUM_VALUE2( FLOAT_MAT3, 0x8B5B );
        ADD_ENUM_VALUE2( FLOAT_MAT4, 0x8B5C );
        ADD_ENUM_VALUE2( FLOAT_MAT2x3, 0x8B65 );
        ADD_ENUM_VALUE2( FLOAT_MAT2x4, 0x8B66 );
        ADD_ENUM_VALUE2( FLOAT_MAT3x2, 0x8B67 );
        ADD_ENUM_VALUE2( FLOAT_MAT3x4, 0x8B68 );
        ADD_ENUM_VALUE2( FLOAT_MAT4x2, 0x8B69 );
        ADD_ENUM_VALUE2( FLOAT_MAT4x3, 0x8B6A );

        ADD_ENUM_VALUE2( DOUBLE_MAT2, 0x8F46 );
        ADD_ENUM_VALUE2( DOUBLE_MAT3, 0x8F47 );
        ADD_ENUM_VALUE2( DOUBLE_MAT4, 0x8F48 );
        ADD_ENUM_VALUE2( DOUBLE_MAT2x3, 0x8F49 );
        ADD_ENUM_VALUE2( DOUBLE_MAT2x4, 0x8F4A );
        ADD_ENUM_VALUE2( DOUBLE_MAT3x2, 0x8F4B );
        ADD_ENUM_VALUE2( DOUBLE_MAT3x4, 0x8F4C );
        ADD_ENUM_VALUE2( DOUBLE_MAT4x2, 0x8F4D );
        ADD_ENUM_VALUE2( DOUBLE_MAT4x3, 0x8F4E );

        ADD_ENUM_VALUE2( SAMPLER_1D, 0x8B5D );
        ADD_ENUM_VALUE2( SAMPLER_2D, 0x8B5E );
        ADD_ENUM_VALUE2( SAMPLER_3D, 0x8B5F );
        ADD_ENUM_VALUE2( SAMPLER_CUBE, 0x8B60 );
        ADD_ENUM_VALUE2( SAMPLER_1D_SHADOW, 0x8B61 );
        ADD_ENUM_VALUE2( SAMPLER_2D_SHADOW, 0x8B62 );
        ADD_ENUM_VALUE2( SAMPLER_1D_ARRAY, 0x8DC0 );
        ADD_ENUM_VALUE2( SAMPLER_2D_ARRAY, 0x8DC1 );
        ADD_ENUM_VALUE2( SAMPLER_CUBE_MAP_ARRAY, 0x900C );
        ADD_ENUM_VALUE2( SAMPLER_1D_ARRAY_SHADOW, 0x8DC3 );
        ADD_ENUM_VALUE2( SAMPLER_2D_ARRAY_SHADOW, 0x8DC4 );
        ADD_ENUM_VALUE2( SAMPLER_2D_MULTISAMPLE, 0x9108 );
        ADD_ENUM_VALUE2( SAMPLER_2D_MULTISAMPLE_ARRAY, 0x910B );
        ADD_ENUM_VALUE2( SAMPLER_CUBE_SHADOW, 0x8DC5 );
        ADD_ENUM_VALUE2( SAMPLER_CUBE_MAP_ARRAY_SHADOW, 0x900D );
        ADD_ENUM_VALUE2( SAMPLER_BUFFER, 0x8DC2 );
        ADD_ENUM_VALUE2( SAMPLER_2D_RECT, 0x8B63 );
        ADD_ENUM_VALUE2( SAMPLER_2D_RECT_SHADOW, 0x8B64 );

        ADD_ENUM_VALUE2( INT_SAMPLER_1D, 0x8DC9 );
        ADD_ENUM_VALUE2( INT_SAMPLER_2D, 0x8DCA );
        ADD_ENUM_VALUE2( INT_SAMPLER_3D, 0x8DCB );
        ADD_ENUM_VALUE2( INT_SAMPLER_CUBE, 0x8DCC );
        ADD_ENUM_VALUE2( INT_SAMPLER_1D_ARRAY, 0x8DCE );
        ADD_ENUM_VALUE2( INT_SAMPLER_2D_ARRAY, 0x8DCF );
        ADD_ENUM_VALUE2( INT_SAMPLER_CUBE_MAP_ARRAY, 0x900E );
        ADD_ENUM_VALUE2( INT_SAMPLER_2D_MULTISAMPLE, 0x9109 );
        ADD_ENUM_VALUE2( INT_SAMPLER_2D_MULTISAMPLE_ARRAY, 0x910C );
        ADD_ENUM_VALUE2( INT_SAMPLER_BUFFER, 0x8DD0 );
        ADD_ENUM_VALUE2( INT_SAMPLER_2D_RECT, 0x8DCD );

        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_1D, 0x8DD1 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_2D, 0x8DD2 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_3D, 0x8DD3 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_CUBE, 0x8DD4 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_1D_ARRAY, 0x8DD6 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_2D_ARRAY, 0x8DD7 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY, 0x900F );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE, 0x910A );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, 0x910D );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_BUFFER, 0x8DD8 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_SAMPLER_2D_RECT, 0x8DD5 );

        ADD_ENUM_VALUE2( IMAGE_1D, 0x904C );
        ADD_ENUM_VALUE2( IMAGE_2D, 0x904D );
        ADD_ENUM_VALUE2( IMAGE_3D, 0x904E );
        ADD_ENUM_VALUE2( IMAGE_2D_RECT, 0x904F );
        ADD_ENUM_VALUE2( IMAGE_CUBE, 0x9050 );
        ADD_ENUM_VALUE2( IMAGE_BUFFER, 0x9051 );
        ADD_ENUM_VALUE2( IMAGE_1D_ARRAY, 0x9052 );
        ADD_ENUM_VALUE2( IMAGE_2D_ARRAY, 0x9053 );
        ADD_ENUM_VALUE2( IMAGE_CUBE_MAP_ARRAY, 0x9054 );
        ADD_ENUM_VALUE2( IMAGE_2D_MULTISAMPLE, 0x9055 );
        ADD_ENUM_VALUE2( IMAGE_2D_MULTISAMPLE_ARRAY, 0x9056 );

        ADD_ENUM_VALUE2( INT_IMAGE_1D, 0x9057 );
        ADD_ENUM_VALUE2( INT_IMAGE_2D, 0x9058 );
        ADD_ENUM_VALUE2( INT_IMAGE_3D, 0x9059 );
        ADD_ENUM_VALUE2( INT_IMAGE_2D_RECT, 0x905A );
        ADD_ENUM_VALUE2( INT_IMAGE_CUBE, 0x905B );
        ADD_ENUM_VALUE2( INT_IMAGE_BUFFER, 0x905C );
        ADD_ENUM_VALUE2( INT_IMAGE_1D_ARRAY, 0x905D );
        ADD_ENUM_VALUE2( INT_IMAGE_2D_ARRAY, 0x905E );
        ADD_ENUM_VALUE2( INT_IMAGE_CUBE_MAP_ARRAY, 0x905F );
        ADD_ENUM_VALUE2( INT_IMAGE_2D_MULTISAMPLE, 0x9060 );
        ADD_ENUM_VALUE2( INT_IMAGE_2D_MULTISAMPLE_ARRAY, 0x9061 );

        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_1D, 0x9062 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_2D, 0x9063 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_3D, 0x9064 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_2D_RECT, 0x9065 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_CUBE, 0x9066 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_BUFFER, 0x9067 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_1D_ARRAY, 0x9068 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_2D_ARRAY, 0x9069 );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY, 0x906A );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_2D_MULTISAMPLE, 0x906B );
        ADD_ENUM_VALUE2( UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, 0x906C );

        ADD_ENUM_VALUE2( UNDEFINED, 0x0 );
    END_ENUM_SERIALIZER();  // _type

    ADD_UINT_SERIALIZER( NumElements, 0 );  // _numElements
    ADD_USER_SERIALIZER( Elements );  // _floatArray, _doubleArray, _intArray, _uintArray
    ADD_USER_WRITER_IF( Elements, hasElements );
    ADD_OBJECT_SERIALIZER( UpdateCallback, osg::UniformCallback, NULL );  // _updateCallback
    ADD_OBJECT_SERIALIZER( EventCallback, osg::UniformCallback, NULL );  // _eventCallback
}
