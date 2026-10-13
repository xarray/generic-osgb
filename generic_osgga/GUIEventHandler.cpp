#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgGA_GUIEventHandler,
                         new osgGA::GUIEventHandler,
                         osgGA::GUIEventHandler,
                         "osg::Object osgGA::GUIEventHandler" )
{
}
