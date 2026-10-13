#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgAnimation_UpdateVec2fUniform,
                         new osgAnimation::UpdateVec2fUniform,
                         osgAnimation::UpdateVec2fUniform,
                         "osg::Object osg::Callback osg::UniformCallback osgAnimation::UpdateVec2fUniform" )
{
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
