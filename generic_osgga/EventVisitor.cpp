#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( EventVisitor,
                         new osgGA::EventVisitor,
                         osgGA::EventVisitor,
                         "osg::Object osg::NodeVisitor osgGA::EventVisitor" )
{
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
