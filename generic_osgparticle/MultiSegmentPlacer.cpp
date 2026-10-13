#include <GenericReserializer.h>
using namespace osgVerse;

static bool readVertices(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::Vec3d vec; is >> vec;
        ud.add("addVertex",  vec );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleMultiSegmentPlacer,
                         new osgParticle::MultiSegmentPlacer,
                         osgParticle::MultiSegmentPlacer,
                         "osg::Object osgParticle::Placer osgParticle::MultiSegmentPlacer" )
{
    ADD_USER_SERIALIZER( Vertices );  // _vx
}
