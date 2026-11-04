#include <GenericReserializer.h>
using namespace osgVerse;

static bool readElevationRange(InputStream& is, InputUserData& ud)
{
    float minElevation, maxElevation, fadeAngle;
    is >> minElevation >> maxElevation >> fadeAngle;
    ud.add("setElevationRange",  minElevation, maxElevation, fadeAngle );
    return true;
}

static bool writeElevationRange(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setElevationRange");
    float a = 0.0f, b = 0.0f, f = 0.0f;
    if (cmd && cmd->args.size() > 2)
    {
        a = (float)irArgDouble(cmd->args[0]); b = (float)irArgDouble(cmd->args[1]);
        f = (float)irArgDouble(cmd->args[2]);
    }
    os << a << b << f << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ElevationSector,
                         new osgSim::ElevationSector,
                         osgSim::ElevationSector,
                         "osg::Object osgSim::Sector osgSim::ElevationSector" )
{
    ADD_USER_SERIALIZER( ElevationRange );  // ElevationRange
    ADD_USER_WRITER( ElevationRange );
}
