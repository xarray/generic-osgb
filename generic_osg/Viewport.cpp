#include <GenericReserializer.h>
using namespace osgVerse;

static bool readArea(InputStream& is, InputUserData& ud)
{
    double x, y, w, h;
    is >> x >> y >> w >> h;
    ud.add("setViewport", x, y, w, h);
    return true;
}

static bool writeArea(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setViewport");
    double x = 0.0, y = 0.0, w = 0.0, h = 0.0;
    if (cmd && cmd->args.size() > 3)
    {
        x = irArgDouble(cmd->args[0]); y = irArgDouble(cmd->args[1]);
        w = irArgDouble(cmd->args[2]); h = irArgDouble(cmd->args[3]);
    }
    os << x << y << w << h << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Viewport,
                         new osg::Viewport,
                         osg::Viewport,
                         "osg::Object osg::StateAttribute osg::Viewport" )
{
    ADD_USER_SERIALIZER( Area );  // _x, _y, _width, _height
    ADD_USER_WRITER( Area );
}
