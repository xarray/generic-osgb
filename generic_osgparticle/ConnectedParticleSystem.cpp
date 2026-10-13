#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleConnectedParticleSystem,
                         new osgParticle::ConnectedParticleSystem,
                         osgParticle::ConnectedParticleSystem,
                         "osg::Object osg::Node osg::Drawable osgParticle::ParticleSystem osgParticle::ConnectedParticleSystem" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::Node")
    }

}
