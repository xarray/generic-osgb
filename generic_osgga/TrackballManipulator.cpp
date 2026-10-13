#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

REGISTER_OBJECT_WRAPPER( osgGA_TrackballManipulator,
                         new osgGA::TrackballManipulator,
                         osgGA::TrackballManipulator,
                         "osg::Object osgGA::TrackballManipulator" )
{
}
