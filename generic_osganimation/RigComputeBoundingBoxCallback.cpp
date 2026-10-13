#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER(osgAnimation_RigComputeBoundingBoxCallback,
                        new osgAnimation::RigComputeBoundingBoxCallback,
                        osgAnimation::RigComputeBoundingBoxCallback,
                        "osg::Object osg::ComputeBoundingBoxCallback osgAnimation::RigComputeBoundingBoxCallback") {
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
