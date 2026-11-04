#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( LightSource,
                         new osg::LightSource,
                         osg::LightSource,
                         "osg::Object osg::Node osg::Group osg::LightSource" )
{
    ADD_OBJECT_SERIALIZER( Light, osg::Light, NULL );  // _light

    BEGIN_ENUM_SERIALIZER( ReferenceFrame, RELATIVE_RF );
        ADD_ENUM_VALUE2( RELATIVE_RF, 0 );
        ADD_ENUM_VALUE2( ABSOLUTE_RF, 1 );
    END_ENUM_SERIALIZER();  // _referenceFrame
}
