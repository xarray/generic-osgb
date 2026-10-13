#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgManipulator_TabBoxTrackballDragger,
                         new osgManipulator::TabBoxTrackballDragger,
                         osgManipulator::TabBoxTrackballDragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger "
                         "osgManipulator::TabBoxTrackballDragger" )  // No need to contain CompositeDragger here
{
}
