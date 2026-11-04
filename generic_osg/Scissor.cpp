#include <GenericReserializer.h>
using namespace osgVerse;

static bool readArea(InputStream& is, InputUserData& ud)
{
    int x, y, w, h;
    is >> x >> y >> w >> h;
    ud.add("setScissor", x, y, w, h);
    return true;
}

static bool writeArea(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setScissor");
    int x = 0, y = 0, w = 0, h = 0;
    if (cmd && cmd->args.size() > 3)
    {
        x = (int)irArgInt(cmd->args[0]); y = (int)irArgInt(cmd->args[1]);
        w = (int)irArgInt(cmd->args[2]); h = (int)irArgInt(cmd->args[3]);
    }
    os << x << y << w << h << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Scissor,
                         new osg::Scissor,
                         osg::Scissor,
                         "osg::Object osg::StateAttribute osg::Scissor" )
{
    ADD_USER_SERIALIZER( Area );  // _x, _y, _width, _height
    ADD_USER_WRITER( Area );
}
