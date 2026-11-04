#include <GenericReserializer.h>
using namespace osgVerse;

// _userDefinedCenter, _radius
static bool readUserCenter(InputStream& is, InputUserData& ud)
{
    osg::Vec3d center; double radius;
    is >> center >> radius;
    ud.add("setCenter", center);
    ud.add("setRadius", radius);
    return true;
}

// _rangeList
static bool readRangeList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float min, max;
        is >> min >> max;
        ud.add("setRange", i, min, max);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeUserCenter(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* center = ud.findCommand("setCenter");
    const IRCommand* radius = ud.findCommand("setRadius");
    osg::Vec3d c;
    double r = 0.0;
    if (center && !center->args.empty()) c = irArgVec3d(center->args[0]);
    if (radius && !radius->args.empty()) r = irArgDouble(radius->args[0]);
    os << c << r << std::endl;
    return true;
}

static bool writeRangeList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setRange", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand& c = *cmds[i];
        float min = c.args.size() > 1 ? (float)irArgDouble(c.args[1]) : 0.0f;
        float max = c.args.size() > 2 ? (float)irArgDouble(c.args[2]) : 0.0f;
        os << min << max << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( LOD,
                         new osg::LOD,
                         osg::LOD,
                         "osg::Object osg::Node osg::Group osg::LOD" )
{
    BEGIN_ENUM_SERIALIZER( CenterMode, USE_BOUNDING_SPHERE_CENTER );
        ADD_ENUM_VALUE2( USE_BOUNDING_SPHERE_CENTER, 0 );
        ADD_ENUM_VALUE2( USER_DEFINED_CENTER, 1 );
        ADD_ENUM_VALUE2( UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED, 2 );
    END_ENUM_SERIALIZER();  // _centerMode

    ADD_USER_SERIALIZER( UserCenter );  // _userDefinedCenter, _radius
    ADD_USER_WRITER( UserCenter );

    BEGIN_ENUM_SERIALIZER( RangeMode, DISTANCE_FROM_EYE_POINT );
        ADD_ENUM_VALUE2( DISTANCE_FROM_EYE_POINT, 0 );
        ADD_ENUM_VALUE2( PIXEL_SIZE_ON_SCREEN, 1 );
    END_ENUM_SERIALIZER();  // _rangeMode

    ADD_USER_SERIALIZER( RangeList );  // _rangeList
    ADD_USER_WRITER( RangeList );
}
