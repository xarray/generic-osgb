#include <GenericReserializer.h>
using namespace osgVerse;

static bool readLightingMap(InputStream& is, InputUserData& ud)
{
    std::string fileName; is.readWrappedString( fileName );
    ud.add("setLightingMap", fileName);
    return true;
}

REGISTER_OBJECT_WRAPPER( osgFX_AnisotropicLighting,
                         new osgFX::AnisotropicLighting,
                         osgFX::AnisotropicLighting,
                         "osg::Object osg::Node osg::Group osgFX::Effect osgFX::AnisotropicLighting" )
{
    ADD_INT_SERIALIZER( LightNumber, 0 );  // _lightnum
    ADD_USER_SERIALIZER( LightingMap );  // _texture
}
