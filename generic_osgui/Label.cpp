#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Label,
                         new osgUI::Label,
                         osgUI::Label,
                         "osg::Object osg::Node osg::Group osgUI::Widget osgUI::Label" )
{
    ADD_STRING_SERIALIZER( Text, std::string());

}
