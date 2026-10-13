#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

REGISTER_OBJECT_WRAPPER( osgGA_UFOManipulator,
                         new osgGA::UFOManipulator,
                         osgGA::UFOManipulator,
                         "osg::Object osgGA::UFOManipulator" )
{
}
