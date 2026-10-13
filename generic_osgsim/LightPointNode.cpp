#include <GenericReserializer.h>
using namespace osgVerse;

static bool readLightPointList(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::Vec3 position; osg::Vec4 color;
        bool on = true; float intensity = 0.0f, radius = 0.0f; int blendingMode = 0;
        is >> is.PROPERTY("LightPoint") >> is.BEGIN_BRACKET;
        is >> is.PROPERTY("Position") >> position;
        is >> is.PROPERTY("Color") >> color;
        is >> is.PROPERTY("Attributes") >> on >> blendingMode >> intensity >> radius;

        ObjectTypeAndID sector, blinkSequence;
        bool hasObject = false; is >> is.PROPERTY("Sector") >> hasObject;
        if ( hasObject )
        {
            is >> is.BEGIN_BRACKET;
            sector = ud.readObjectFromStream(is, "osgSim::Sector");
            is >> is.END_BRACKET;
        }
        hasObject = false; is >> is.PROPERTY("BlinkSequence") >> hasObject;
        if ( hasObject )
        {
            is >> is.BEGIN_BRACKET;
            blinkSequence = ud.readObjectFromStream(is, "osgSim::BlinkSequence");
            is >> is.END_BRACKET;
        }
        is >> is.END_BRACKET;
        ud.add("addLightPoint", position, color, on, blendingMode, intensity, radius,
               &sector, &blinkSequence);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_LightPointNode,
                         new osgSim::LightPointNode,
                         osgSim::LightPointNode,
                         "osg::Object osg::Node osgSim::LightPointNode" )
{
    ADD_USER_SERIALIZER( LightPointList );  // _lightPointList
    ADD_FLOAT_SERIALIZER( MinPixelSize, 0.0f );  // _minPixelSize
    ADD_FLOAT_SERIALIZER( MaxPixelSize, 30.0f );  // _maxPixelSize
    ADD_FLOAT_SERIALIZER( MaxVisibleDistance2, FLT_MAX );  // _maxVisibleDistance2
    ADD_OBJECT_SERIALIZER( LightPointSystem, osgSim::LightPointSystem, NULL );  // _lightSystem
    ADD_BOOL_SERIALIZER( PointSprite, false );  // _pointSprites
}
