#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgAnimation_UpdateBone,
                         new osgAnimation::UpdateBone,
                         osgAnimation::UpdateBone,
                         "osg::Object osg::Callback osg::NodeCallback osgAnimation::UpdateMatrixTransform osgAnimation::UpdateBone" )
{
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
