#include <GenericReserializer.h>
using namespace osgVerse;

static bool readPlacers(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID p = ud.readObjectFromStream(is, "osgParticle::Placer");
        if ( p ) ud.add("addPlacer",  p.get() );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleCompositePlacer,
                         new osgParticle::CompositePlacer,
                         osgParticle::CompositePlacer,
                         "osg::Object osgParticle::Placer osgParticle::CompositePlacer" )
{
    ADD_USER_SERIALIZER( Placers );  // _placers
}
