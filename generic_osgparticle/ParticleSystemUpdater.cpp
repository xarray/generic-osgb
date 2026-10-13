#include <GenericReserializer.h>
using namespace osgVerse;

static bool readParticleSystems(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID ps = ud.readObjectFromStream(is, "osgParticle::ParticleSystem");
        if ( ps ) ud.add("addParticleSystem",  ps.get() );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleParticleSystemUpdater,
                         new osgParticle::ParticleSystemUpdater,
                         osgParticle::ParticleSystemUpdater,
                         "osg::Object osg::Node osgParticle::ParticleSystemUpdater" )
{
    ADD_USER_SERIALIZER( ParticleSystems );  // _psv
}
