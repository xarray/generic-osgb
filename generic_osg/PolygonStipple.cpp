#include <GenericReserializer.h>
using namespace osgVerse;

static bool readMask(InputStream& is, InputUserData& ud)
{
    char mask[128] = {0};
    if ( is.isBinary() )
    {
        unsigned int size; is >> size;
        is.readCharArray( mask, size );
    }
    else
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<128; ++i )
        {
            is >> std::hex >> mask[i] >> std::dec;
        }
        is >> is.END_BRACKET;
    }
    // Record the 128 mask bytes so the write side can reproduce them (the
    // pointer form below would produce an empty IR argument).
    ud.add("setMask", std::string(mask, 128));
    return true;
}

static bool writeMask(OutputStream& os, OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArg("Mask");
    std::string mask = a ? irArgString(*a) : std::string();
    mask.resize(128, '\0');

    if ( os.isBinary() )
    {
        os << (unsigned int)128;
        os.writeCharArray(mask.data(), 128);
    }
    else
    {
        os << os.BEGIN_BRACKET << std::endl;
        for ( unsigned int i=0; i<128; ++i )
        {
            os << std::hex << (int)(unsigned char)mask[i] << std::dec << std::endl;
        }
        os << os.END_BRACKET << std::endl;
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( PolygonStipple,
                         new osg::PolygonStipple,
                         osg::PolygonStipple,
                         "osg::Object osg::StateAttribute osg::PolygonStipple" )
{
    ADD_USER_SERIALIZER( Mask );  // _mask
    ADD_USER_WRITER( Mask );
}
