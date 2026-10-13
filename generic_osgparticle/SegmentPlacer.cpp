#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgParticleSegmentPlacer,
                         new osgParticle::SegmentPlacer,
                         osgParticle::SegmentPlacer,
                         "osg::Object osgParticle::Placer osgParticle::SegmentPlacer" )
{
    ADD_VEC3_SERIALIZER( VertexA, osg::Vec3() );  // _vertexA
    ADD_VEC3_SERIALIZER( VertexB, osg::Vec3() );  // _vertexB
}
