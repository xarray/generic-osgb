#include <GenericReserializer.h>
using namespace osgVerse;

static bool readMasks(InputStream& is, InputUserData& ud)
{
    if ( is.getFileVersion() > 96 )
    {
        unsigned int mask0, mask1;
        is >> mask0 >> mask1;
        ud.add("setMask", mask0, 0);
        ud.add("setMask", mask1, 1);
    }
    return true;
}

static bool writeMasks(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd0 = ud.findCommandWithInt("setMask", 0, 1);
    const IRCommand* cmd1 = ud.findCommandWithInt("setMask", 1, 1);
    unsigned int mask0 = (cmd0 && cmd0->args.size() > 1) ? (unsigned int)irArgInt(cmd0->args[0]) : 0;
    unsigned int mask1 = (cmd1 && cmd1->args.size() > 1) ? (unsigned int)irArgInt(cmd1->args[0]) : 0;
    os << mask0 << mask1 << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( SampleMaski,
                         new osg::SampleMaski,
                         osg::SampleMaski,
                         "osg::Object osg::StateAttribute osg::SampleMaski" )
{
    ADD_USER_SERIALIZER( Masks );  //
    ADD_USER_WRITER( Masks );
}
