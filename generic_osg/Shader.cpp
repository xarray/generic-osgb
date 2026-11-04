#include <GenericReserializer.h>
#include <sstream>
using namespace osgVerse;

static bool readShaderSource(InputStream& is, InputUserData& ud)
{
    std::string code;
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string line;
        is.readWrappedString( line );
        code.append( line ); code.append( 1, '\n' );
    }
    is >> is.END_BRACKET;
    ud.add("setShaderSource", code);
    return true;
}

static bool hasShaderSource(OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArg("ShaderSource");
    return a && !irArgString(*a).empty();
}

static bool writeShaderSource(OutputStream& os, OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArg("ShaderSource");
    std::string code = a ? irArgString(*a) : std::string();

    std::vector<std::string> lines;
    std::istringstream iss(code);
    std::string line;
    while (std::getline(iss, line)) lines.push_back(line);

    os << (unsigned int)lines.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        os.writeWrappedString(lines[i]);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Shader,
                         new osg::Shader,
                         osg::Shader,
                         "osg::Object osg::Shader" )
{
    BEGIN_ENUM_SERIALIZER( Type, UNDEFINED);
        ADD_ENUM_VALUE2( VERTEX, 0x8B31 );
        ADD_ENUM_VALUE2( TESSCONTROL, 0x8E88 );
        ADD_ENUM_VALUE2( TESSEVALUATION, 0x8E87 );
        ADD_ENUM_VALUE2( FRAGMENT, 0x8B30 );
        ADD_ENUM_VALUE2( GEOMETRY, 0x8DD9 );
        ADD_ENUM_VALUE2( COMPUTE, 0x91B9 );
        ADD_ENUM_VALUE2( UNDEFINED, -1 );
    END_ENUM_SERIALIZER();  // _type

    ADD_USER_SERIALIZER( ShaderSource );  // _shaderSource
    ADD_USER_WRITER_IF( ShaderSource, hasShaderSource );
    ADD_OBJECT_SERIALIZER( ShaderBinary, osg::ShaderBinary, NULL );  // _shaderBinary
}
