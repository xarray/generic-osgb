#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleForceOperator,
                         new osgParticle::ForceOperator,
                         osgParticle::ForceOperator,
                         "osg::Object osgParticle::Operator osgParticle::ForceOperator" )
{
    ADD_VEC3_SERIALIZER( Force, osg::Vec3() );  // _force
}
