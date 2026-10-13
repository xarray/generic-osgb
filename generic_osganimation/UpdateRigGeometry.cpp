#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER(osgAnimation_UpdateRigGeometry,
                        new osgAnimation::UpdateRigGeometry,
                        osgAnimation::UpdateRigGeometry,
                        "osg::Object osg::Callback osg::UpdateCallback osgAnimation::UpdateRigGeometry") {}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
