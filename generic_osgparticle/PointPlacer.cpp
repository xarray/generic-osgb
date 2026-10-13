#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticlePointPlacer,
                         new osgParticle::PointPlacer,
                         osgParticle::PointPlacer,
                         "osg::Object osgParticle::Placer osgParticle::CenteredPlacer osgParticle::PointPlacer" )
{
}
