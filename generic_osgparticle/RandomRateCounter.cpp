#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleRandomRateCounter,
                         new osgParticle::RandomRateCounter,
                         osgParticle::RandomRateCounter,
                         "osg::Object osgParticle::Counter osgParticle::VariableRateCounter osgParticle::RandomRateCounter" )
{
}
