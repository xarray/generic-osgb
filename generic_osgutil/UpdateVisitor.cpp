#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( UpdateVisitor,
                         new osgUtil::UpdateVisitor,
                         osgUtil::UpdateVisitor,
                         "osg::Object osg::NodeVisitor osgUtil::UpdateVisitor" )
{
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
