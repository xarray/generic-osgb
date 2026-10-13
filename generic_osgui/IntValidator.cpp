#include <GenericReserializer.h>
using namespace osgVerse;

#include <limits.h>

REGISTER_OBJECT_WRAPPER( IntValidator,
                         new osgUI::IntValidator,
                         osgUI::IntValidator,
                         "osg::Object osgUI::Validator osgUI::IntValidator" )
{
    ADD_INT_SERIALIZER(Bottom, -INT_MAX);
    ADD_INT_SERIALIZER(Top, INT_MAX);
}
