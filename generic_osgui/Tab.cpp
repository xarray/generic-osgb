#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Tab,
                         new osgUI::Tab,
                         osgUI::Tab,
                         "osg::Object osgUI::Tab" )
{
    ADD_STRING_SERIALIZER(Text, "");
    ADD_OBJECT_SERIALIZER(Widget, osgUI::Widget, NULL);
}
