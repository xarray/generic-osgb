#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgSim_LightPointSystem,
                         new osgSim::LightPointSystem,
                         osgSim::LightPointSystem,
                         "osg::Object osgSim::LightPointSystem" )
{
    ADD_FLOAT_SERIALIZER( Intensity, 1.0f );  // _intensity
    BEGIN_ENUM_SERIALIZER( AnimationState, ANIMATION_ON );
        ADD_ENUM_VALUE( ANIMATION_ON );
        ADD_ENUM_VALUE( ANIMATION_OFF );
        ADD_ENUM_VALUE( ANIMATION_RANDOM );
    END_ENUM_SERIALIZER();  // _animationState
}
