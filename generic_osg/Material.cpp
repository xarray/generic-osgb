#include <GenericReserializer.h>
using namespace osgVerse;

#define MATERIAL_FUNC( PROP, TYPE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        bool frontAndBack; TYPE value1, value2; \
        is >> frontAndBack; \
        is >> is.PROPERTY("Front") >> value1; \
        is >> is.PROPERTY("Back") >> value2; \
        if ( frontAndBack ) \
            ud.add("set" #PROP, GL_FRONT_AND_BACK, value1); \
        else { \
            ud.add("set" #PROP, GL_FRONT, value1); \
            ud.add("set" #PROP, GL_BACK, value2); \
        } \
        return true; \
    }

MATERIAL_FUNC( Ambient, osg::Vec4f )
MATERIAL_FUNC( Diffuse, osg::Vec4f )
MATERIAL_FUNC( Specular, osg::Vec4f )
MATERIAL_FUNC( Emission, osg::Vec4f )
MATERIAL_FUNC( Shininess, float )

template<typename T> static T materialArgValue(const IRArg& a);
template<> osg::Vec4f materialArgValue<osg::Vec4f>(const IRArg& a) { return irArgVec4f(a); }
template<> float materialArgValue<float>(const IRArg& a) { return (float)irArgDouble(a); }

#define MATERIAL_WRITER( PROP, TYPE ) \
    static bool write##PROP(OutputStream& os, OutputUserData& ud) { \
        const IRCommand* both = ud.findCommandWithInt("set" #PROP, GL_FRONT_AND_BACK, 0); \
        TYPE front = TYPE(), back = TYPE(); bool frontAndBack = (both != NULL); \
        if (both) { \
            if (both->args.size() > 1) front = back = materialArgValue<TYPE>(both->args[1]); \
        } else { \
            const IRCommand* f = ud.findCommandWithInt("set" #PROP, GL_FRONT, 0); \
            const IRCommand* b = ud.findCommandWithInt("set" #PROP, GL_BACK, 0); \
            if (f && f->args.size() > 1) front = materialArgValue<TYPE>(f->args[1]); \
            if (b && b->args.size() > 1) back = materialArgValue<TYPE>(b->args[1]); \
        } \
        os << frontAndBack; \
        os << os.PROPERTY("Front") << front; \
        os << os.PROPERTY("Back") << back; \
        os << std::endl; \
        return true; \
    }

MATERIAL_WRITER( Ambient, osg::Vec4f )
MATERIAL_WRITER( Diffuse, osg::Vec4f )
MATERIAL_WRITER( Specular, osg::Vec4f )
MATERIAL_WRITER( Emission, osg::Vec4f )
MATERIAL_WRITER( Shininess, float )

REGISTER_OBJECT_WRAPPER( Material,
                         new osg::Material,
                         osg::Material,
                         "osg::Object osg::StateAttribute osg::Material" )
{
    BEGIN_ENUM_SERIALIZER( ColorMode, OFF );
        ADD_ENUM_VALUE2( AMBIENT, 0x1200 );
        ADD_ENUM_VALUE2( DIFFUSE, 0x1201 );
        ADD_ENUM_VALUE2( SPECULAR, 0x1202 );
        ADD_ENUM_VALUE2( EMISSION, 0x1600 );
        ADD_ENUM_VALUE2( AMBIENT_AND_DIFFUSE, 0x1602 );
        ADD_ENUM_VALUE2( OFF, 0x1603 );
    END_ENUM_SERIALIZER();  // _colorMode

    ADD_USER_SERIALIZER( Ambient );  // _ambient
    ADD_USER_WRITER( Ambient );
    ADD_USER_SERIALIZER( Diffuse );  // _diffuse
    ADD_USER_WRITER( Diffuse );
    ADD_USER_SERIALIZER( Specular );  // _specular
    ADD_USER_WRITER( Specular );
    ADD_USER_SERIALIZER( Emission );  // _emission
    ADD_USER_WRITER( Emission );
    ADD_USER_SERIALIZER( Shininess );  // _shininess
    ADD_USER_WRITER( Shininess );
}
