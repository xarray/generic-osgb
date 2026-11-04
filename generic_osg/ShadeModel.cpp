#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ShadeModel,
                         new osg::ShadeModel,
                         osg::ShadeModel,
                         "osg::Object osg::StateAttribute osg::ShadeModel" )
{
    BEGIN_ENUM_SERIALIZER( Mode, SMOOTH );
        ADD_ENUM_VALUE2( FLAT, 0x1D00 );
        ADD_ENUM_VALUE2( SMOOTH, 0x1D01 );
    END_ENUM_SERIALIZER();  // _mode
}
