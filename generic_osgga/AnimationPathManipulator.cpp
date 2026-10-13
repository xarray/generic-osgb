#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

REGISTER_OBJECT_WRAPPER( osgGA_AnimationPathManipulator,
                         new osgGA::AnimationPathManipulator,
                         osgGA::AnimationPathManipulator,
                         "osg::Object osgGA::AnimationPathManipulator" )
{
}
