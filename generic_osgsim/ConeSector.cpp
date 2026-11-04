#include <GenericReserializer.h>
using namespace osgVerse;

static bool readAngle(InputStream& is, InputUserData& ud)
{
    float angle, fadeAngle;
    is >> angle >> fadeAngle;
    ud.add("setAngle",  angle, fadeAngle );
    return true;
}

static bool writeAngle(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setAngle");
    float a = 0.0f, f = 0.0f;
    if (cmd && cmd->args.size() > 1)
    {
        a = (float)irArgDouble(cmd->args[0]); f = (float)irArgDouble(cmd->args[1]);
    }
    os << a << f << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ConeSector,
                         new osgSim::ConeSector,
                         osgSim::ConeSector,
                         "osg::Object osgSim::Sector osgSim::ConeSector" )
{
    ADD_VEC3_SERIALIZER( Axis, osg::Vec3() );  // _axis
    ADD_USER_SERIALIZER( Angle );  // _cosAngle
    ADD_USER_WRITER( Angle );
}
