#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Dialog,
                         new osgUI::Dialog,
                         osgUI::Dialog,
                         "osg::Object osg::Node osg::Group osgUI::Widget osgUI::Dialog" )
{
    ADD_STRING_SERIALIZER( Title, std::string());
}
