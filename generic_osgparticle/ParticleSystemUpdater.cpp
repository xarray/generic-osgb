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

static bool writeParticleSystems(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addParticleSystem", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        int target = cmds[i]->args.empty() ? -1 : ud.resolve(cmds[i]->args[0]);
        if (os.isBinary()) os << (target >= 0);
        os.writeObject(ud, target);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleParticleSystemUpdater,
                         new osgParticle::ParticleSystemUpdater,
                         osgParticle::ParticleSystemUpdater,
                         "osg::Object osg::Node osgParticle::ParticleSystemUpdater" )
{
    ADD_USER_SERIALIZER( ParticleSystems );  // _psv
    ADD_USER_WRITER( ParticleSystems );
}
