#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleModularEmitter,
                         new osgParticle::ModularEmitter,
                         osgParticle::ModularEmitter,
                         "osg::Object osg::Node osgParticle::ParticleProcessor osgParticle::Emitter osgParticle::ModularEmitter" )
{
    ADD_OBJECT_SERIALIZER( Counter, osgParticle::Counter, NULL );  // _counter
    ADD_OBJECT_SERIALIZER( Placer, osgParticle::Placer, NULL );  // _placer
    ADD_OBJECT_SERIALIZER( Shooter, osgParticle::Shooter, NULL );  // _shooter
}
