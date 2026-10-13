#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleAngularAccelOperator,
                         new osgParticle::AngularAccelOperator,
                         osgParticle::AngularAccelOperator,
                         "osg::Object osgParticle::Operator osgParticle::AngularAccelOperator" )
{
    ADD_VEC3_SERIALIZER( AngularAcceleration, osg::Vec3() );  // _angul_araccel
}
