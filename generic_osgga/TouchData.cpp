#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgGA_TouchData,
                         new osgGA::GUIEventAdapter::TouchData,
                         osgGA::GUIEventAdapter::TouchData,
                         "osg::Object osgGA::TouchData" )
{
}
