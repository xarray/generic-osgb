#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgAnimation_Skeleton,
                         new osgAnimation::Skeleton,
                         osgAnimation::Skeleton,
                         "osg::Object osg::Node osg::Group osg::Transform osg::MatrixTransform osgAnimation::Skeleton" )
{
}
