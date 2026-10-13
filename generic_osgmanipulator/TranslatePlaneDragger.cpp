#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgManipulator_TranslatePlaneDragger,
                         new osgManipulator::TranslatePlaneDragger,
                         osgManipulator::TranslatePlaneDragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger "
                         "osgManipulator::TranslatePlaneDragger" )  // No need to contain CompositeDragger here
{
}
