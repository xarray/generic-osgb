#include <GenericReserializer.h>
using namespace osgVerse;

static bool readAzimRange(InputStream& is, InputUserData& ud)
{
    float minAzimuth, maxAzimuth, fadeAngle;
    is >> minAzimuth >> maxAzimuth >> fadeAngle;
    ud.add("setAzimuthRange",  minAzimuth, maxAzimuth, fadeAngle );
    return true;
}

static bool writeAzimRange(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setAzimuthRange");
    float a = 0.0f, b = 0.0f, f = 0.0f;
    if (cmd && cmd->args.size() > 2)
    {
        a = (float)irArgDouble(cmd->args[0]); b = (float)irArgDouble(cmd->args[1]);
        f = (float)irArgDouble(cmd->args[2]);
    }
    os << a << b << f << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_AzimSector,
                         new osgSim::AzimSector,
                         osgSim::AzimSector,
                         "osg::Object osgSim::Sector osgSim::AzimSector" )
{
    ADD_USER_SERIALIZER( AzimRange );  // AzimRange
    ADD_USER_WRITER( AzimRange );
}
