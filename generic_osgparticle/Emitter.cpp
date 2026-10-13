#include <GenericReserializer.h>
using namespace osgVerse;

extern bool readParticle( InputStream& is, InputUserData& ud );

static bool readParticleTemplate(InputStream& is, InputUserData& ud)
{
    ud.add("setParticleTemplate");
    readParticle( is, ud );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleEmitter,
                         /*new osgParticle::Emitter*/NULL,
                         osgParticle::Emitter,
                         "osg::Object osg::Node osgParticle::ParticleProcessor osgParticle::Emitter" )
{
    ADD_BOOL_SERIALIZER( UseDefaultTemplate, true );  // _usedeftemp
    ADD_USER_SERIALIZER( ParticleTemplate );  // _ptemp
}
