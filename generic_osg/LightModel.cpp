#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( LightModel,
                         new osg::LightModel,
                         osg::LightModel,
                         "osg::Object osg::StateAttribute osg::LightModel" )
{
    ADD_VEC4_SERIALIZER( AmbientIntensity, osg::Vec4() );  // _ambient

    BEGIN_ENUM_SERIALIZER( ColorControl, SINGLE_COLOR );
        ADD_ENUM_VALUE2( SEPARATE_SPECULAR_COLOR, 0 );
        ADD_ENUM_VALUE2( SINGLE_COLOR, 1 );
    END_ENUM_SERIALIZER();  // _colorControl

    ADD_BOOL_SERIALIZER( LocalViewer, false );  // _localViewer
    ADD_BOOL_SERIALIZER( TwoSided, false );  // _twoSided
}
