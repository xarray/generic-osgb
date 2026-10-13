#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleBounceOperator,
                         new osgParticle::BounceOperator,
                         osgParticle::BounceOperator,
                         "osg::Object osgParticle::Operator osgParticle::DomainOperator osgParticle::BounceOperator" )
{
    ADD_FLOAT_SERIALIZER( Friction, 1.0f );  // _friction
    ADD_FLOAT_SERIALIZER( Resilience, 0.0f );  // _resilience
    ADD_FLOAT_SERIALIZER( Cutoff, 0.0f );  // _cutoff
}
