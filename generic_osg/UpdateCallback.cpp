#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER2(UpdateCallback,
                        new osg::DrawableUpdateCallback,
                        osg::DrawableUpdateCallback,
                        "osg::UpdateCallback",
                        "osg::Object osg::Callback osg::UpdateCallback") {}
