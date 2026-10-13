#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleFireEffect,
                         new osgParticle::FireEffect,
                         osgParticle::FireEffect,
                         "osg::Object osg::Node osg::Group osgParticle::ParticleEffect osgParticle::FireEffect" )
{
}
