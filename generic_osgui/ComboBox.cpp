#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ComboBox,
                         new osgUI::ComboBox,
                         osgUI::ComboBox,
                         "osg::Object osg::Node osg::Group osgUI::Widget osgUI::ComboBox" )
{
    ADD_UINT_SERIALIZER(CurrentIndex, 0);
    ADD_VECTOR_SERIALIZER( Items, osgUI::ComboBox::Items, osgDB::BaseSerializer::RW_OBJECT, 0 );
}
