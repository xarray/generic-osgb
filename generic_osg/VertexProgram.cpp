#include <GenericReserializer.h>
using namespace osgVerse;

// _programLocalParameters
static bool readLocalParameters(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        unsigned int key; osg::Vec4d value;
        is >> key >> value;
        ud.add("setProgramLocalParameter", key, value);
    }
    is >> is.END_BRACKET;
    return true;
}

// _matrixList
static bool readMatrices(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        unsigned int key; osg::Matrixd value;
        is >> key >> value;
        ud.add("setMatrix", key, value);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeLocalParameters(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setProgramLocalParameter", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 2 ) continue;
        os << (unsigned int)irArgInt(c.args[0]) << irArgVec4d(c.args[1]) << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeMatrices(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setMatrix", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 2 ) continue;
        osg::Matrixd m;
        for ( int k=0; k<16; ++k ) m._m[k] = c.args[1].v[k];
        os << (unsigned int)irArgInt(c.args[0]) << m << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( VertexProgram,
                         new osg::VertexProgram,
                         osg::VertexProgram,
                         "osg::Object osg::StateAttribute osg::VertexProgram" )
{
    ADD_STRING_SERIALIZER( VertexProgram, "" );  // _fragmentProgram
    ADD_USER_SERIALIZER( LocalParameters );  // _programLocalParameters
    ADD_USER_WRITER( LocalParameters );
    ADD_USER_SERIALIZER( Matrices );  // _matrixList
    ADD_USER_WRITER( Matrices );
}
