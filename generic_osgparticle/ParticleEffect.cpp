#include <GenericReserializer.h>
using namespace osgVerse;

// _particleSystem

static bool readParticleSystem(InputStream& is, InputUserData& ud)
{
    is >> is.BEGIN_BRACKET;
    ud.add("setUseLocalParticleSystem",  false );
    ObjectTypeAndID ps = ud.readObjectFromStream(is, "osgParticle::ParticleSystem");
    ud.add("setParticleSystem",  ps.get() );
    is >> is.END_BRACKET;
    return true;
}

static bool writeParticleSystem(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setParticleSystem");
    int target = (cmd && !cmd->args.empty()) ? ud.resolve(cmd->args[0]) : -1;

    os << os.BEGIN_BRACKET << std::endl;
    if (os.isBinary()) os << (target >= 0);
    if (target >= 0) os.writeObject(ud, target);
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleParticleEffect,
                         /*new osgParticle::ParticleEffect*/NULL,
                         osgParticle::ParticleEffect,
                         "osg::Object osg::Node osg::Group osgParticle::ParticleEffect" )
{
    ADD_USER_SERIALIZER( ParticleSystem );  // _particleSystem
    ADD_USER_WRITER( ParticleSystem );
    ADD_STRING_SERIALIZER( TextureFileName, "" );  // _textureFileName
    ADD_VEC3_SERIALIZER( Position, osg::Vec3() );  // _position
    ADD_FLOAT_SERIALIZER( Scale, 0.0f );  // _scale
    ADD_FLOAT_SERIALIZER( Intensity, 0.0f );  // _intensity
    ADD_DOUBLE_SERIALIZER( StartTime, 0.0 );  // _startTime
    ADD_DOUBLE_SERIALIZER( EmitterDuration, 0.0 );  // _emitterDuration
    ADD_VEC3_SERIALIZER( Wind, osg::Vec3() );  // _wind
}
