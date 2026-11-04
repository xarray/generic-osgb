#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER2( ComputeBoundingBoxCallback,
                         new osg::Drawable::ComputeBoundingBoxCallback,
                         osg::Drawable::ComputeBoundingBoxCallback,
                         "osg::ComputeBoundingBoxCallback",
                         "osg::Object osg::ComputeBoundingBoxCallback" )
{
}
