#include <GenericReserializer.h>
using namespace osgVerse;

static bool readOperators(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID op = ud.readObjectFromStream(is, "osgParticle::Operator");
        if ( op ) ud.add("addOperator",  op.get() );
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeOperators(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addOperator", cmds);

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

REGISTER_OBJECT_WRAPPER( osgParticleModularProgram,
                         new osgParticle::ModularProgram,
                         osgParticle::ModularProgram,
                         "osg::Object osg::Node osgParticle::ParticleProcessor osgParticle::Program osgParticle::ModularProgram" )
{
    ADD_USER_SERIALIZER( Operators );  // _operators
    ADD_USER_WRITER( Operators );
}
