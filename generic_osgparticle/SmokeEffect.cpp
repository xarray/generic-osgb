#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleSmokeEffect,
                         new osgParticle::SmokeEffect,
                         osgParticle::SmokeEffect,
                         "osg::Object osg::Node osg::Group osgParticle::ParticleEffect osgParticle::SmokeEffect" )
{
}
