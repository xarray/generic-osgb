#include <GenericReserializer.h>
using namespace osgVerse;

static bool readPositionList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize();
    is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::Vec3d pos; is >> pos;
        ud.add("readPositionList", i, pos);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool hasPositionList(OutputUserData& ud)
{ return ud.countCommands("readPositionList") > 0; }

static bool writePositionList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("readPositionList", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        osg::Vec3d pos;
        if (cmds[i]->args.size() > 1) pos = irArgVec3d(cmds[i]->args[1]);
        os << pos << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Billboard,
                         new osg::Billboard,
                         osg::Billboard,
                         "osg::Object osg::Node osg::Geode osg::Billboard" )
{
    BEGIN_ENUM_SERIALIZER( Mode, AXIAL_ROT );
        ADD_ENUM_VALUE2( POINT_ROT_EYE, 0 );
        ADD_ENUM_VALUE2( POINT_ROT_WORLD, 1 );
        ADD_ENUM_VALUE2( AXIAL_ROT, 2 );
    END_ENUM_SERIALIZER();  // _mode

    ADD_VEC3_SERIALIZER( Axis, osg::Vec3f() );  // _axis
    ADD_VEC3_SERIALIZER( Normal, osg::Vec3f() );  // _normal
    ADD_USER_SERIALIZER( PositionList );  // _positionList
    ADD_USER_WRITER_IF( PositionList, hasPositionList );
}
