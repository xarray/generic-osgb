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

static bool writePlacers(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addPlacer", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.empty() ) continue;
        if (os.isBinary()) os << (ud.resolve(c.args[0]) >= 0);
        os.writeObject(ud, ud.resolve(c.args[0]));
        if (!os.isBinary()) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleCompositePlacer,
                         new osgParticle::CompositePlacer,
                         osgParticle::CompositePlacer,
                         "osg::Object osgParticle::Placer osgParticle::CompositePlacer" )
{
    ADD_USER_SERIALIZER( Placers );  // _placers
    ADD_USER_WRITER( Placers );
}
