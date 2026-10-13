#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleAccelOperator,
                         new osgParticle::AccelOperator,
                         osgParticle::AccelOperator,
                         "osg::Object osgParticle::Operator osgParticle::AccelOperator" )
{
    ADD_VEC3_SERIALIZER( Acceleration, osg::Vec3() );  // _accel
}
