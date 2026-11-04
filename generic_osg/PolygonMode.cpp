#include <GenericReserializer.h>
using namespace osgVerse;

#ifndef GL_VERSION_1_0
#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_FRONT_AND_BACK                 0x0408
#define GL_POINT                          0x1B00
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02
#endif

enum osg_PolygonMode_Mode { POINT = GL_POINT, LINE = GL_LINE, FILL = GL_FILL };
BEGIN_USER_TABLE( Mode, osg_PolygonMode );
    ADD_USER_VALUE( POINT );
    ADD_USER_VALUE( LINE );
    ADD_USER_VALUE( FILL );
END_USER_TABLE()

USER_READ_FUNC( Mode, readModeValue )
USER_WRITE_FUNC( Mode, writeModeValue )

// _modeFront, _modeBack
static bool readMode(InputStream& is, InputUserData& ud)
{
    bool frontAndBack;
    is >> is.PROPERTY("UseFrontAndBack") >> frontAndBack;
    is >> is.PROPERTY("Front"); int value1 = readModeValue(is);
    is >> is.PROPERTY("Back"); int value2 = readModeValue(is);

    if ( frontAndBack )
        ud.add("setMode", GL_FRONT_AND_BACK, value1);
    else
    {
        ud.add("setMode", GL_FRONT, value1);
        ud.add("setMode", GL_BACK, value2);
    }
    return true;
}

static bool writeMode(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setMode", cmds);

    int front = GL_FILL, back = GL_FILL;
    bool frontAndBack = false;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand* cmd = cmds[i];
        if (cmd->args.size() < 2) continue;
        int face = (int)irArgInt(cmd->args[0]);
        int mode = (int)irArgInt(cmd->args[1]);
        if (face == GL_FRONT_AND_BACK) { front = back = mode; frontAndBack = true; }
        else if (face == GL_FRONT) front = mode;
        else if (face == GL_BACK) back = mode;
    }

    os << os.PROPERTY("UseFrontAndBack") << frontAndBack << std::endl;
    os << os.PROPERTY("Front");
    writeModeValue(os, front);
    os << std::endl;
    os << os.PROPERTY("Back");
    writeModeValue(os, back);
    os << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( PolygonMode,
                         new osg::PolygonMode,
                         osg::PolygonMode,
                         "osg::Object osg::StateAttribute osg::PolygonMode" )
{
    ADD_USER_SERIALIZER( Mode );  // _modeFront, _modeBack
    ADD_USER_WRITER( Mode );
}
