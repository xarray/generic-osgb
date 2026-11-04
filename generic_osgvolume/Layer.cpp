#include <GenericReserializer.h>
using namespace osgVerse;

#define FILTER_FUNCTIONS( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        DEF_GLENUM(mode); is >> mode; \
        ud.add( "set" #PROP, mode.get() ); \
        return true; \
    } \
    static bool write##PROP( OutputStream& os, OutputUserData& ud ) { \
        const IRArg* a = ud.findPropertyArg(#PROP); \
        unsigned int mode = a ? (unsigned int)irArgInt(*a) : 0; \
        os << ObjectGLenum(mode, "GL") << std::endl; \
        return true; \
    }

FILTER_FUNCTIONS( MinFilter )
FILTER_FUNCTIONS( MagFilter )

REGISTER_OBJECT_WRAPPER( osgVolume_Layer,
                         new osgVolume::Layer,
                         osgVolume::Layer,
                         "osg::Object osgVolume::Layer" )
{
    ADD_STRING_SERIALIZER( FileName, "" );  // _filename
    ADD_OBJECT_SERIALIZER( Locator, osgVolume::Locator, NULL );  // _locator
    ADD_VEC4_SERIALIZER( DefaultValue, osg::Vec4() );  // _defaultValue
    ADD_USER_SERIALIZER( MinFilter );  // _minFilter
    ADD_USER_WRITER( MinFilter );
    ADD_USER_SERIALIZER( MagFilter );  // _magFilter
    ADD_USER_WRITER( MagFilter );
    ADD_OBJECT_SERIALIZER( Property, osgVolume::Property, NULL );  // _property
}
