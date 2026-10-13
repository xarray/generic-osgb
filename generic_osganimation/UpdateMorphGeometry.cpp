#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER(osgAnimation_UpdateMorphGeometry,
                        new osgAnimation::UpdateMorphGeometry,
                        osgAnimation::UpdateMorphGeometry,
                        "osg::Object osg::Callback osg::UpdateCallback osgAnimation::UpdateMorphGeometry") {}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
