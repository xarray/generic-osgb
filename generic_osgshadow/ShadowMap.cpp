#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgShadow_ShadowMap,
                         new osgShadow::ShadowMap,
                         osgShadow::ShadowMap,
                         "osg::Object osgShadow::ShadowTechnique osgShadow::ShadowMap" )
{
    ADD_UINT_SERIALIZER( TextureUnit, 1 );  // _shadowTextureUnit
    ADD_VEC2_SERIALIZER( PolygonOffset, osg::Vec2f() );  // _polyOffset
    ADD_VEC2D_SERIALIZER( AmbientBias, osg::Vec2d() );  // _ambientBias

    ADD_VEC2S_SERIALIZER( TextureSize, osg::Vec2s(1024, 1024) );  // _textureSize
}
