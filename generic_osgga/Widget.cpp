#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgGA_Widget,
                         new osgGA::Widget,
                         osgGA::Widget,
                         "osg::Object osg::Node osg::Group osgGA::Widget" )
{
    BEGIN_ENUM_SERIALIZER( FocusBehaviour, FOCUS_FOLLOWS_POINTER );
        ADD_ENUM_VALUE( CLICK_TO_FOCUS );
        ADD_ENUM_VALUE( FOCUS_FOLLOWS_POINTER );
        ADD_ENUM_VALUE( EVENT_DRIVEN_FOCUS_DISABLED );
    END_ENUM_SERIALIZER();

    ADD_BOOL_SERIALIZER(HasEventFocus, false);

    ADD_BOUNDINGBOXF_SERIALIZER(Extents, osg::BoundingBoxf());

}
