#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( AlphaFunc,
                         new osg::AlphaFunc,
                         osg::AlphaFunc,
                         "osg::Object osg::StateAttribute osg::AlphaFunc" )
{
    BEGIN_ENUM_SERIALIZER2( Function, osg::AlphaFunc::ComparisonFunction, ALWAYS );
        ADD_ENUM_VALUE2( NEVER, 0x0200 );
        ADD_ENUM_VALUE2( LESS, 0x0201 );
        ADD_ENUM_VALUE2( EQUAL, 0x0202 );
        ADD_ENUM_VALUE2( LEQUAL, 0x0203 );
        ADD_ENUM_VALUE2( GREATER, 0x0204 );
        ADD_ENUM_VALUE2( NOTEQUAL, 0x0205 );
        ADD_ENUM_VALUE2( GEQUAL, 0x0206 );
        ADD_ENUM_VALUE2( ALWAYS, 0x0207 );
    END_ENUM_SERIALIZER();  // _comparisonFunc

    ADD_FLOAT_SERIALIZER( ReferenceValue, 1.0f );  // _referenceValue
}
