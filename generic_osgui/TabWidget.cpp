#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( TabWidget,
                         new osgUI::TabWidget,
                         osgUI::TabWidget,
                         "osg::Object osg::Node osg::Group osgUI::Widget osgUI::TabWidget" )
{
    ADD_UINT_SERIALIZER(CurrentIndex, 0);
    ADD_VECTOR_SERIALIZER( Tabs, osgUI::TabWidget::Tabs, osgDB::BaseSerializer::RW_OBJECT, 0 );
}
