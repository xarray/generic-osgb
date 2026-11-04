#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( TexEnv,
                         new osg::TexEnv,
                         osg::TexEnv,
                         "osg::Object osg::StateAttribute osg::TexEnv" )
{
    BEGIN_ENUM_SERIALIZER( Mode, MODULATE );
        ADD_ENUM_VALUE2( DECAL, 0x2101 );
        ADD_ENUM_VALUE2( MODULATE, 0x2100 );
        ADD_ENUM_VALUE2( BLEND, 0x0BE2 );
        ADD_ENUM_VALUE2( REPLACE, 0x1E01 );
        ADD_ENUM_VALUE2( ADD, 0x0104 );
    END_ENUM_SERIALIZER();  // _mode

    ADD_VEC4_SERIALIZER( Color, osg::Vec4() );  // _color
}
