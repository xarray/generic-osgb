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

REGISTER_OBJECT_WRAPPER( Widget,
                         new osgUI::Widget,
                         osgUI::Widget,
                         "osg::Object osg::Node osg::Group osgUI::Widget" )
{
    BEGIN_ENUM_SERIALIZER( FocusBehaviour, FOCUS_FOLLOWS_POINTER );
        ADD_ENUM_VALUE( CLICK_TO_FOCUS );
        ADD_ENUM_VALUE( FOCUS_FOLLOWS_POINTER );
        ADD_ENUM_VALUE( EVENT_DRIVEN_FOCUS_DISABLED );
    END_ENUM_SERIALIZER();

    ADD_BOOL_SERIALIZER(HasEventFocus, false);

    ADD_USER_SERIALIZER(GraphicsSubgraphMap);
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
