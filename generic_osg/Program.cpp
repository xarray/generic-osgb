#include <GenericReserializer.h>
using namespace osgVerse;

#define PROGRAM_LIST_FUNC( PROP, TYPE, DATA ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET; \
        for ( unsigned int i=0; i<size; ++i ) { \
            std::string key; unsigned int value; \
            is >> key >> value; ud.add("add" #DATA, key, value); \
        } \
        is >> is.END_BRACKET; \
        return true; \
    }

PROGRAM_LIST_FUNC( AttribBinding, AttribBindingList, BindAttribLocation )
PROGRAM_LIST_FUNC( FragDataBinding, FragDataBindingList, BindFragDataLocation )

#define PROGRAM_LIST_WRITER( PROP, DATA ) \
    static bool has##PROP(OutputUserData& ud) \
    { return ud.countCommands("add" #DATA) > 0; } \
    static bool write##PROP(OutputStream& os, OutputUserData& ud) { \
        std::vector<const IRCommand*> cmds; \
        ud.findCommands("add" #DATA, cmds); \
        os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl; \
        for (size_t i = 0; i < cmds.size(); ++i) { \
            const std::vector<IRArg>& a = cmds[i]->args; \
            os << irArgString(a.size() > 0 ? a[0] : IRArg()) \
               << (unsigned int)(a.size() > 1 ? irArgInt(a[1]) : 0) << std::endl; \
        } \
        os << os.END_BRACKET << std::endl; \
        return true; \
    }

PROGRAM_LIST_WRITER( AttribBinding, BindAttribLocation )
PROGRAM_LIST_WRITER( FragDataBinding, BindFragDataLocation )

#define PROGRAM_PARAMETER_FUNC( PROP, NAME ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        int value; is >> is.PROPERTY(#NAME) >> value; \
        ud.add("setParameter", NAME, value); \
        return true; \
    } \
    static bool has##PROP(OutputUserData& ud) \
    { return ud.findCommandWithInt("setParameter", NAME, 0) != NULL; } \
    static bool write##PROP(OutputStream& os, OutputUserData& ud) { \
        const IRCommand* cmd = ud.findCommandWithInt("setParameter", NAME, 0); \
        int value = (cmd && cmd->args.size() > 1) ? (int)irArgInt(cmd->args[1]) : 0; \
        os << os.PROPERTY(#NAME) << value << std::endl; \
        return true; \
    }

PROGRAM_PARAMETER_FUNC( GeometryVerticesOut, GL_GEOMETRY_VERTICES_OUT_EXT )
PROGRAM_PARAMETER_FUNC( GeometryInputType, GL_GEOMETRY_INPUT_TYPE_EXT )
PROGRAM_PARAMETER_FUNC( GeometryOutputType, GL_GEOMETRY_OUTPUT_TYPE_EXT )

// _shaderList
static bool readShaders(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID shader = is.readObject(ud);
        if ( shader.valid() ) ud.add("addShader", shader);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool hasShaders(OutputUserData& ud)
{ return ud.countCommands("addShader") > 0; }

static bool writeShaders(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addShader", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        int target = cmds[i]->args.empty() ? -1 : ud.resolve(cmds[i]->args[0]);
        os.writeObject(ud, target);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

// feedBackVaryings
static bool readFeedBackVaryingsName(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string str; is>> str;
        ud.add("addTransformFeedBackVarying", str);
    }
    is >> is.END_BRACKET;
    return true;
}

// feedBack mode
static bool readFeedBackMode(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is>>size;
    ud.add("setTransformFeedBackMode", size);
    return true;
}

static bool writeFeedBackVaryingsName(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addTransformFeedBackVarying", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
        os << irArgString(cmds[i]->args.empty() ? IRArg() : cmds[i]->args[0]) << std::endl;
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeFeedBackMode(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setTransformFeedBackMode");
    unsigned int value = (cmd && !cmd->args.empty()) ? (unsigned int)irArgInt(cmd->args[0]) : 0;
    os << value << std::endl;
    return true;
}

static bool writeBindUniformBlock(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addBindUniformBlock", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const std::vector<IRArg>& a = cmds[i]->args;
        os << irArgString(a.size() > 0 ? a[0] : IRArg())
           << (unsigned int)(a.size() > 1 ? irArgInt(a[1]) : 0) << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

// _numGroupsX/Y/Z
static bool readComputeGroups(InputStream& is, InputUserData& ud)
{
    GLint numX = 0, numY = 0, numZ = 0;
    is >> numX >> numY >> numZ;
    return true;
}

static bool readBindUniformBlock(InputStream& is, InputUserData& ud)
{
    unsigned int  size = 0; is >> size >> is.BEGIN_BRACKET;
    std::string name; unsigned int index;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >>name; is >>index;    
        ud.add("addBindUniformBlock", name, index);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( Program,
                         new osg::Program,
                         osg::Program,
                         "osg::Object osg::StateAttribute osg::Program" )
{
    ADD_USER_SERIALIZER( AttribBinding );  // _attribBindingList
    ADD_USER_WRITER_IF( AttribBinding, hasAttribBinding );
    ADD_USER_SERIALIZER( FragDataBinding );  // _fragDataBindingList
    ADD_USER_WRITER_IF( FragDataBinding, hasFragDataBinding );
    ADD_USER_SERIALIZER( Shaders );  // _shaderList
    ADD_USER_WRITER_IF( Shaders, hasShaders );
    ADD_USER_SERIALIZER( GeometryVerticesOut );  // _geometryVerticesOut
    ADD_USER_WRITER( GeometryVerticesOut );
    ADD_USER_SERIALIZER( GeometryInputType );  // _geometryInputType
    ADD_USER_WRITER( GeometryInputType );
    ADD_USER_SERIALIZER( GeometryOutputType );  // _geometryOutputType
    ADD_USER_WRITER( GeometryOutputType );

    {
        UPDATE_TO_VERSION_SCOPED( 95 )
        ADD_USER_SERIALIZER( ComputeGroups );  // _numGroupsX/Y/Z
    }

    {
        UPDATE_TO_VERSION_SCOPED( 153 )
        REMOVE_SERIALIZER( ComputeGroups );
    }

    {
        UPDATE_TO_VERSION_SCOPED( 116 )
        ADD_USER_SERIALIZER( FeedBackVaryingsName );
        ADD_USER_WRITER( FeedBackVaryingsName );
        ADD_USER_SERIALIZER( FeedBackMode );
        ADD_USER_WRITER( FeedBackMode );
    }
    {
        UPDATE_TO_VERSION_SCOPED( 150 )
        ADD_USER_SERIALIZER( BindUniformBlock );
        ADD_USER_WRITER( BindUniformBlock );
    }
}
