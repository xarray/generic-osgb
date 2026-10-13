#include <GenericReserializer.h>
using namespace osgVerse;

static bool readAzimRange(InputStream& is, InputUserData& ud)
{
    float minAzimuth, maxAzimuth, fadeAngle;
    is >> minAzimuth >> maxAzimuth >> fadeAngle;
    ud.add("setAzimuthRange",  minAzimuth, maxAzimuth, fadeAngle );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_AzimSector,
                         new osgSim::AzimSector,
                         osgSim::AzimSector,
                         "osg::Object osgSim::Sector osgSim::AzimSector" )
{
    ADD_USER_SERIALIZER( AzimRange );  // AzimRange
}
