#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgAnimation_StackedTranslateElement,
                         new osgAnimation::StackedTranslateElement,
                         osgAnimation::StackedTranslateElement,
                         "osg::Object osgAnimation::StackedTransformElement osgAnimation::StackedTranslateElement" )
{
    ADD_VEC3_SERIALIZER( Translate, osg::Vec3() );  // _translate
}
