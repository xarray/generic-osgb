#include <GenericReserializer.h>
using namespace osgVerse;

#include <limits.h>

REGISTER_OBJECT_WRAPPER( DoubleValidator,
                         new osgUI::DoubleValidator,
                         osgUI::DoubleValidator,
                         "osg::Object osgUI::Validator osgUI::DoubleValidator" )
{
    ADD_INT_SERIALIZER(Decimals, -1);
    ADD_DOUBLE_SERIALIZER(Bottom, -DBL_MAX);
    ADD_DOUBLE_SERIALIZER(Top, DBL_MAX);
}
