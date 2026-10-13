#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

REGISTER_OBJECT_WRAPPER( osgGA_MultiTouchTrackballManipulator,
                         new osgGA::MultiTouchTrackballManipulator,
                         osgGA::MultiTouchTrackballManipulator,
                         "osg::Object osgGA::MultiTouchTrackballManipulator" )
{
}
