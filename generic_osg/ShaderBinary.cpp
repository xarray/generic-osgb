#include <GenericReserializer.h>
using namespace osgVerse;

static bool readData(InputStream& is, InputUserData& ud)
{
    unsigned int size; is >> size;
    char* data = new char[size]();
    if ( is.isBinary() )
    {
        is.readCharArray( data, size );
    }
    else
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            is >> std::hex >> data[i] >> std::dec;
        }
        is >> is.END_BRACKET;
    }

    if (size>0)
    {
        // Keep the bytes in the IR so the write side can reproduce them.
        ud.add("assign", size, std::string(data, size));
    }

    delete [] data;
    return true;
}

static bool writeData(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("assign");
    std::string data = (cmd && cmd->args.size() > 1) ? irArgString(cmd->args[1]) : std::string();
    unsigned int size = (unsigned int)data.size();

    if ( os.isBinary() )
    {
        os << (unsigned int)size;
        os.writeCharArray(data.data(), size);
    }
    else
    {
        os << (unsigned int)size;
        os << os.BEGIN_BRACKET << std::endl;
        for ( unsigned int i=0; i<size; ++i )
        {
            os << std::hex << (int)(unsigned char)data[i] << std::dec << std::endl;
        }
        os << os.END_BRACKET << std::endl;
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( ShaderBinary,
                         new osg::ShaderBinary,
                         osg::ShaderBinary,
                         "osg::Object osg::ShaderBinary" )
{
    ADD_USER_SERIALIZER( Data );  // _data
    ADD_USER_WRITER( Data );
}
