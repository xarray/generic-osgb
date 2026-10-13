#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( CullVisitor,
                         new osgUtil::CullVisitor,
                         osgUtil::CullVisitor,
                         "osg::Object osg::NodeVisitor osgUtil::CullVisitor" )
{
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
