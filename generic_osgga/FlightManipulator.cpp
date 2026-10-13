#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

REGISTER_OBJECT_WRAPPER( osgGA_FlightManipulator,
                         new osgGA::FlightManipulator,
                         osgGA::FlightManipulator,
                         "osg::Object osgGA::FlightManipulator" )
{
}
