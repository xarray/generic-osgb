#include <GenericReserializer.h>
using namespace osgVerse;

// _databasePath
static bool readDatabasePath(InputStream& is, InputUserData& ud)
{
    bool hasPath; is >> hasPath;
    if ( !hasPath )
    {
        /*if (is.getOptions() && !is.getOptions()->getDatabasePathList().empty())
        {
            const std::string& optionPath = is.getOptions()->getDatabasePathList().front();
            if ( !optionPath.empty() ) ud.add("setDatabasePath", optionPath);
        }*/
    }
    else
    {
        std::string path; is.readWrappedString( path );
        ud.add( "setDatabasePath", path );
    }
    return true;
}

// _perRangeDataList
static bool readRangeDataList(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string name; is.readWrappedString( name );
        ud.add("setFileName", i, name);
    }
    is >> is.END_BRACKET;

    size = 0; is >> is.PROPERTY("PriorityList") >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float offset, scale;
        is >> offset >> scale;
        ud.add("setPriorityOffset", i, offset);
        ud.add("setPriorityScale", i, scale);
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

static bool writeDatabasePath(OutputStream& os, OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArg("DatabasePath");
    bool hasPath = (a != NULL);
    os << hasPath;
    if (hasPath) os.writeWrappedString(irArgString(*a));
    os << std::endl;
    return true;
}

static bool writeRangeDataList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> names, offsets, scales;
    ud.findCommands("setFileName", names);
    ud.findCommands("setPriorityOffset", offsets);
    ud.findCommands("setPriorityScale", scales);

    os << (unsigned int)names.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const IRCommand& c = *names[i];
        os.writeWrappedString(c.args.size() > 1 ? irArgString(c.args[1]) : std::string());
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;

    os << os.PROPERTY("PriorityList") << (unsigned int)offsets.size()
       << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < offsets.size(); ++i)
    {
        float offset = offsets[i]->args.size() > 1
            ? (float)irArgDouble(offsets[i]->args[1]) : 0.0f;
        float scale = (i < scales.size() && scales[i]->args.size() > 1)
            ? (float)irArgDouble(scales[i]->args[1]) : 1.0f;
        os << offset << scale << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeChildren(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> names, children;
    ud.findCommands("setFileName", names);
    ud.findCommands("addChild", children);

    unsigned int dynamicLoaded = 0;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const IRCommand& c = *names[i];
        if (c.args.size() > 1 && !irArgString(c.args[1]).empty()) ++dynamicLoaded;
    }
    unsigned int realSize = (unsigned int)names.size() - dynamicLoaded;

    os << realSize;
    if (realSize > 0)
    {
        os << os.BEGIN_BRACKET << std::endl;
        size_t child = 0;
        for (size_t i = 0; i < names.size() && child < children.size(); ++i)
        {
            const IRCommand& c = *names[i];
            if (c.args.size() > 1 && !irArgString(c.args[1]).empty()) continue;
            int index = children[child]->args.empty() ? -1 : ud.resolve(children[child]->args[0]);
            os.writeObject(ud, index);
            os << std::endl;
            ++child;
        }
        os << os.END_BRACKET;
    }
    os << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( PagedLOD,
                         new osg::PagedLOD,
                         osg::PagedLOD,
                         "osg::Object osg::Node osg::LOD osg::PagedLOD" )
{
    // Note: osg::Group is not in the list to prevent recording dynamic loaded children

    ADD_USER_SERIALIZER( DatabasePath );  // _databasePath
    ADD_USER_WRITER( DatabasePath );
    ADD_UINT_SERIALIZER( FrameNumberOfLastTraversal, 0 );  // _frameNumberOfLastTraversal, note, not required, removed from soversion 70 onwwards, see below
    ADD_UINT_SERIALIZER( NumChildrenThatCannotBeExpired, 0 );  // _numChildrenThatCannotBeExpired
    ADD_BOOL_SERIALIZER( DisableExternalChildrenPaging, false );  // _disableExternalChildrenPaging
    ADD_USER_SERIALIZER( RangeDataList );  // _perRangeDataList
    ADD_USER_WRITER( RangeDataList );
    ADD_USER_SERIALIZER( Children );  // _children (which are not loaded from external)
    ADD_USER_WRITER( Children );

    {
        UPDATE_TO_VERSION_SCOPED( 70 )
        REMOVE_SERIALIZER( FrameNumberOfLastTraversal );
    }



}
