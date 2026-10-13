#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( LineEdit,
                         new osgUI::LineEdit,
                         osgUI::LineEdit,
                         "osg::Object osg::Node osg::Group osgUI::Widget osgUI::LineEdit" )
{
    ADD_OBJECT_SERIALIZER( Validator, osgUI::Validator, NULL);
    ADD_STRING_SERIALIZER( Text, std::string());
}
