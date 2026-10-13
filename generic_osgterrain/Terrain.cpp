#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgTerrain_Terrain,
                         new osgTerrain::Terrain,
                         osgTerrain::Terrain,
                         "osg::Object osg::Node osg::Group osg::CoordinateSystemNode osgTerrain::Terrain" )
{
    ADD_FLOAT_SERIALIZER( SampleRatio, 1.0f );  // _sampleRatio
    ADD_FLOAT_SERIALIZER( VerticalScale, 1.0f );  // _verticalScale

    // Note: osg uses BEGIN_ENUM_SERIALIZER4 / ADD_ENUM_CLASS_VALUE for the
    // scoped enum here; the generic scheme spells the values out by token.
    BEGIN_ENUM_SERIALIZER( BlendingPolicy, INHERIT );
        ADD_ENUM_VALUE( INHERIT );
        ADD_ENUM_VALUE( DO_NOT_SET_BLENDING );
        ADD_ENUM_VALUE( ENABLE_BLENDING );
        ADD_ENUM_VALUE( ENABLE_BLENDING_WHEN_ALPHA_PRESENT );
    END_ENUM_SERIALIZER();  // BlendingPolicy
}
