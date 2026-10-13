#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( PushButton,
                         new osgUI::PushButton,
                         osgUI::PushButton,
                         "osg::Object osg::Node osg::Group osgUI::Widget osgUI::PushButton" )
{
    ADD_STRING_SERIALIZER( Text, std::string());
}
