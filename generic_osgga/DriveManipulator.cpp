#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

REGISTER_OBJECT_WRAPPER( osgGA_DriveManipulator,
                         new osgGA::DriveManipulator,
                         osgGA::DriveManipulator,
                         "osg::Object osgGA::DriveManipulator" )
{
}
