#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgManipulator_TabPlaneTrackballDragger,
                         new osgManipulator::TabPlaneTrackballDragger,
                         osgManipulator::TabPlaneTrackballDragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger "
                         "osgManipulator::TabPlaneTrackballDragger" )  // No need to contain CompositeDragger here
{
}
