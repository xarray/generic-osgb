#include <GenericReserializer.h>
using namespace osgVerse;

// _graphicsSubgraphMap: a map<unsigned int, osg::Object*> written as a sized,
// bracketed list of key / object pairs.
static bool readGraphicsSubgraphMap(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i = 0; i < size; ++i )
    {
        unsigned int key = 0; is >> key;
        ObjectTypeAndID value = is.readObject(ud);
        if ( value.valid() ) ud.add("setGraphicsSubgraph", key, &value);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeGraphicsSubgraphMap(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setGraphicsSubgraph", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if (c.args.size() < 2) continue;
        os << (unsigned int)irArgInt(c.args[0]);
        os.writeObject(ud, ud.resolve(c.args[1]));
        if ( !os.isBinary() ) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Widget,
                         new osgUI::Widget,
                         osgUI::Widget,
                         "osg::Object osg::Node osg::Group osgUI::Widget" )
{
    BEGIN_ENUM_SERIALIZER( FocusBehaviour, FOCUS_FOLLOWS_POINTER );
        ADD_ENUM_VALUE2( CLICK_TO_FOCUS, 0 );
        ADD_ENUM_VALUE2( FOCUS_FOLLOWS_POINTER, 1 );
        ADD_ENUM_VALUE2( EVENT_DRIVEN_FOCUS_DISABLED, 2 );
    END_ENUM_SERIALIZER();

    ADD_BOOL_SERIALIZER(HasEventFocus, false);

    ADD_USER_SERIALIZER(GraphicsSubgraphMap);
    ADD_USER_WRITER(GraphicsSubgraphMap);
    SET_USAGE( osgDB::BaseSerializer::GET_SET_PROPERTY);

    ADD_OBJECT_SERIALIZER( WidgetStateSet, osg::StateSet, NULL );
    SET_USAGE( osgDB::BaseSerializer::GET_SET_PROPERTY);

    ADD_BOUNDINGBOXF_SERIALIZER(Extents, osg::BoundingBoxf());

    ADD_OBJECT_SERIALIZER( FrameSettings, osgUI::FrameSettings, NULL );
    ADD_OBJECT_SERIALIZER( AlignmentSettings, osgUI::AlignmentSettings, NULL );
    ADD_OBJECT_SERIALIZER( TextSettings, osgUI::TextSettings, NULL );

    ADD_BOOL_SERIALIZER(Visible, true);
    ADD_BOOL_SERIALIZER(Enabled, true);

}
