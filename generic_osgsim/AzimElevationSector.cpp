#include <GenericReserializer.h>
using namespace osgVerse;

static bool readAzimRange(InputStream& is, InputUserData& ud)
{
    float minAzimuth, maxAzimuth, fadeAngle;
    is >> minAzimuth >> maxAzimuth >> fadeAngle;
    ud.add("setAzimuthRange",  minAzimuth, maxAzimuth, fadeAngle );
    return true;
}

static bool readElevationRange(InputStream& is, InputUserData& ud)
{
    float minElevation, maxElevation, fadeAngle;
    is >> minElevation >> maxElevation >> fadeAngle;
    ud.add("setElevationRange",  minElevation, maxElevation, fadeAngle );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_AzimElevationSector,
                         new osgSim::AzimElevationSector,
                         osgSim::AzimElevationSector,
                         "osg::Object osgSim::Sector osgSim::AzimElevationSector" )
{
    ADD_USER_SERIALIZER( AzimRange );  // AzimRange
    ADD_USER_SERIALIZER( ElevationRange );  // ElevationRange
}
