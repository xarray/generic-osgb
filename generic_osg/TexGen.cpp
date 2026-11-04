#include <GenericReserializer.h>
using namespace osgVerse;

enum osg_TexGen_Coord { S, T, R, Q };
#define PLANE_FUNCTION( PROP, COORD ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        osg::Plane plane; is >> plane; \
        ud.add("setPlane", COORD, plane); \
        return true; \
    } \
    static bool write##PROP(OutputStream& os, OutputUserData& ud) { \
        const IRCommand* cmd = ud.findCommandWithInt("setPlane", COORD, 0); \
        osg::Vec4d v; \
        if (cmd && cmd->args.size() > 1) v = irArgVec4d(cmd->args[1]); \
        os << osg::Plane(v._v[0], v._v[1], v._v[2], v._v[3]) << std::endl; \
        return true; \
    }

PLANE_FUNCTION( PlaneS, osg_TexGen_Coord::S )
PLANE_FUNCTION( PlaneT, osg_TexGen_Coord::T )
PLANE_FUNCTION( PlaneR, osg_TexGen_Coord::R )
PLANE_FUNCTION( PlaneQ, osg_TexGen_Coord::Q )

REGISTER_OBJECT_WRAPPER( TexGen,
                         new osg::TexGen,
                         osg::TexGen,
                         "osg::Object osg::StateAttribute osg::TexGen" )
{
    BEGIN_ENUM_SERIALIZER( Mode, OBJECT_LINEAR );
        ADD_ENUM_VALUE2( OBJECT_LINEAR, 0x2400 );
        ADD_ENUM_VALUE2( EYE_LINEAR, 0x2401 );
        ADD_ENUM_VALUE2( SPHERE_MAP, 0x2402 );
        ADD_ENUM_VALUE2( NORMAL_MAP, 0x8511 );
        ADD_ENUM_VALUE2( REFLECTION_MAP, 0x8512 );
    END_ENUM_SERIALIZER();  // _mode

    ADD_USER_SERIALIZER_RW( PlaneS );
    ADD_USER_SERIALIZER_RW( PlaneT );
    ADD_USER_SERIALIZER_RW( PlaneR );
    ADD_USER_SERIALIZER_RW( PlaneQ );  //_plane_s, _plane_t, _plane_r, _plane_q
}
