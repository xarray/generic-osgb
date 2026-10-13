#include <GenericReserializer.h>
using namespace osgVerse;

static bool readAngle(InputStream& is, InputUserData& ud)
{
    float angle, fadeAngle;
    is >> angle >> fadeAngle;
    ud.add("setAngle",  angle, fadeAngle );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ConeSector,
                         new osgSim::ConeSector,
                         osgSim::ConeSector,
                         "osg::Object osgSim::Sector osgSim::ConeSector" )
{
    ADD_VEC3_SERIALIZER( Axis, osg::Vec3() );  // _axis
    ADD_USER_SERIALIZER( Angle );  // _cosAngle
}
