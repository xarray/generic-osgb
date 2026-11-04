#include <GenericReserializer.h>
using namespace osgVerse;

// _filenameList
static bool readFileNames(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string value;
        is.readWrappedString( value );
        ud.add("setFileName", i, value);
    }
    is >> is.END_BRACKET;
    return true;
}

// _children
static bool readChildren(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size;
    if (size > 0)
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            ObjectTypeAndID child = is.readObject(ud);
            if ( child.valid() ) ud.add("addChild", &child);
        }
        is >> is.END_BRACKET;
    }
    return true;
}

// _userDefinedCenter, _radius
static bool readUserCenter(InputStream& is, InputUserData& ud)
{
    osg::Vec3d center; double radius;
    is >> center >> radius;
    ud.add("setCenter", center);
    ud.add("setRadius", radius);
    return true;
}

static bool writeFileNames(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setFileName", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( unsigned int i=0; i<(unsigned int)cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        os.writeWrappedString(c.args.size() > 1 ? irArgString(c.args[1]) : std::string());
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeChildren(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addChild", cmds);

    os << (unsigned int)cmds.size();
    if (cmds.empty())
    {
        if (!os.isBinary()) os << std::endl;
        return true;
    }
    os << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.empty() ) continue;
        os.writeObject(ud, ud.resolve(c.args[0]));
        if (!os.isBinary()) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeUserCenter(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* center = ud.findCommand("setCenter");
    const IRCommand* radius = ud.findCommand("setRadius");
    osg::Vec3d c;
    if (center && !center->args.empty()) c = irArgVec3d(center->args[0]);
    double r = (radius && !radius->args.empty()) ? irArgDouble(radius->args[0]) : 0.0;
    os << c << r << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( ProxyNode,
                         new osg::ProxyNode,
                         osg::ProxyNode,
                         "osg::Object osg::Node osg::ProxyNode" )
{
    // Note: osg::Group is not in the list to prevent recording dynamic loaded children

    ADD_USER_SERIALIZER( FileNames );  // _filenameList
    ADD_USER_WRITER( FileNames );
    ADD_USER_SERIALIZER( Children );  // _children (which are not loaded from external)
    ADD_USER_WRITER( Children );
    ADD_STRING_SERIALIZER( DatabasePath, "" );  // _databasePath

    BEGIN_ENUM_SERIALIZER( LoadingExternalReferenceMode, LOAD_IMMEDIATELY );
        ADD_ENUM_VALUE2( LOAD_IMMEDIATELY, 0 );
        ADD_ENUM_VALUE2( DEFER_LOADING_TO_DATABASE_PAGER, 1 );
        ADD_ENUM_VALUE2( NO_AUTOMATIC_LOADING, 2 );
    END_ENUM_SERIALIZER();  // _loadingExtReference

    BEGIN_ENUM_SERIALIZER( CenterMode, USE_BOUNDING_SPHERE_CENTER );
        ADD_ENUM_VALUE2( USE_BOUNDING_SPHERE_CENTER, 0 );
        ADD_ENUM_VALUE2( USER_DEFINED_CENTER, 1 );
        ADD_ENUM_VALUE2( UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED, 2 );
    END_ENUM_SERIALIZER();  // _centerMode

    ADD_USER_SERIALIZER( UserCenter );  // _userDefinedCenter, _radius
    ADD_USER_WRITER( UserCenter );

    //wrapper->addFinishedObjectReadCallback(new ProxyNodeFinishedObjectReadCallback());
}
