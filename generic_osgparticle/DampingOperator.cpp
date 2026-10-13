#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleDampingOperator,
                         new osgParticle::DampingOperator,
                         osgParticle::DampingOperator,
                         "osg::Object osgParticle::Operator osgParticle::DampingOperator" )
{
    ADD_VEC3_SERIALIZER( Damping, osg::Vec3() );  // _damping
    ADD_FLOAT_SERIALIZER( CutoffLow, 0.0f );  // _cutoffLow
    ADD_FLOAT_SERIALIZER( CutoffHigh, FLT_MAX );  // _cutoffHigh
}
