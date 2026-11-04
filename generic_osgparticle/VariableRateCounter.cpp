#include <GenericReserializer.h>
using namespace osgVerse;

static bool readRateRange(InputStream& is, InputUserData& ud)
{
    float min, max; is >> min >> max;
    ud.add("setRateRange",  min, max ); return true;
}

static bool writeRateRange(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setRateRange");
    float min = 0.0f, max = 0.0f;
    if (cmd && cmd->args.size() > 1)
    {
        min = (float)irArgDouble(cmd->args[0]);
        max = (float)irArgDouble(cmd->args[1]);
    }
    os << min << max << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleVariableRateCounter,
                         /*new osgParticle::VariableRateCounter*/NULL,
                         osgParticle::VariableRateCounter,
                         "osg::Object osgParticle::Counter osgParticle::VariableRateCounter" )
{
    ADD_USER_SERIALIZER( RateRange );  // _rate_range
    ADD_USER_WRITER( RateRange );
}
