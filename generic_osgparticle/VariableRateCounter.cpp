#include <GenericReserializer.h>
using namespace osgVerse;

static bool readRateRange(InputStream& is, InputUserData& ud)
{
    float min, max; is >> min >> max;
    ud.add("setRateRange",  min, max ); return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleVariableRateCounter,
                         /*new osgParticle::VariableRateCounter*/NULL,
                         osgParticle::VariableRateCounter,
                         "osg::Object osgParticle::Counter osgParticle::VariableRateCounter" )
{
    ADD_USER_SERIALIZER( RateRange );  // _rate_range
}
