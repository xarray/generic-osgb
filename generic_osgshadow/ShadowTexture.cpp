#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgShadow_ShadowTexture,
                         new osgShadow::ShadowTexture,
                         osgShadow::ShadowTexture,
                         "osg::Object osgShadow::ShadowTechnique osgShadow::ShadowTexture" )
{
    ADD_UINT_SERIALIZER( TextureUnit, 1 );  // _textureUnit
}
