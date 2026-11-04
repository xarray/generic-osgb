#include <GenericReserializer.h>
using namespace osgVerse;

static bool readRestartIndex(InputStream& is, InputUserData& ud)
{
    if ( is.getFileVersion() > 97 )
    {
        unsigned int restartIndex;
        is >> restartIndex;
        ud.add("setRestartIndex", restartIndex);
    }
    return true;
}

static bool writeRestartIndex(OutputStream& os, OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArg("RestartIndex");
    os << (unsigned int)(a ? irArgInt(*a) : 0) << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( PrimitiveRestartIndex,
                         new osg::PrimitiveRestartIndex,
                         osg::PrimitiveRestartIndex,
                         "osg::Object osg::StateAttribute osg::PrimitiveRestartIndex" )
{
    ADD_USER_SERIALIZER( RestartIndex );
    ADD_USER_WRITER( RestartIndex );
}
