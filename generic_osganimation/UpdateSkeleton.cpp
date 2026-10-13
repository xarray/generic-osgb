#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER2( osgAnimation_UpdateSkeleton,
                          new osgAnimation::Skeleton::UpdateSkeleton,
                          osgAnimation::Skeleton::UpdateSkeleton,
                          "osgAnimation::UpdateSkeleton",
                          "osg::Object osg::Callback osg::NodeCallback osgAnimation::UpdateSkeleton" )
{
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
