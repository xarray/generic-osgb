#include <GenericReserializer.h>
using namespace osgVerse;

static bool readElevationRange(InputStream& is, InputUserData& ud)
{
    float minElevation, maxElevation, fadeAngle;
    is >> minElevation >> maxElevation >> fadeAngle;
    ud.add("setElevationRange",  minElevation, maxElevation, fadeAngle );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ElevationSector,
                         new osgSim::ElevationSector,
                         osgSim::ElevationSector,
                         "osg::Object osgSim::Sector osgSim::ElevationSector" )
{
    ADD_USER_SERIALIZER( ElevationRange );  // ElevationRange
}
