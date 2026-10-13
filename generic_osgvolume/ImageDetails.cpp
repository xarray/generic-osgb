#include <GenericReserializer.h>
using namespace osgVerse;

static bool readMatrix(InputStream& is, InputUserData& ud)
{
    osg::Matrixd matrix; is >> matrix;
    ud.add("setMatrix", matrix);
    return true;
}

REGISTER_OBJECT_WRAPPER( osgVolume_ImageDetails,
                         new osgVolume::ImageDetails,
                         osgVolume::ImageDetails,
                         "osg::Object osgVolume::ImageDetails" )
{
    ADD_VEC4_SERIALIZER( TexelOffset, osg::Vec4() );  // _texelOffset
    ADD_VEC4_SERIALIZER( TexelScale, osg::Vec4() );  // _texelScale
    ADD_USER_SERIALIZER( Matrix );  // _matrix
}
