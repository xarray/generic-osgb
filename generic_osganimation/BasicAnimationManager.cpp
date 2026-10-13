#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;
namespace osgAnimation_BasicAnimationManagerWrapper{
REGISTER_OBJECT_WRAPPER( osgAnimation_BasicAnimationManager,
                         new osgAnimation::BasicAnimationManager,
                         osgAnimation::BasicAnimationManager,
                         "osg::Object osg::NodeCallback osgAnimation::AnimationManagerBase osgAnimation::BasicAnimationManager" )
{

}
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
