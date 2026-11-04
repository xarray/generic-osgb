#include <GenericReserializer.h>
using namespace osgVerse;

#define RADIALSHOOTER_FLOAT_FUNCTION( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        float min = 0.0f, max = 0.0f; is >> min >> max; \
        ud.add( "set" #PROP, min, max ); return true; \
    } \
    static bool write##PROP( OutputStream& os, OutputUserData& ud ) { \
        const IRCommand* cmd = ud.findCommand("set" #PROP); \
        float min = 0.0f, max = 0.0f; \
        if (cmd && cmd->args.size() > 1) { \
            min = (float)irArgDouble(cmd->args[0]); max = (float)irArgDouble(cmd->args[1]); } \
        os << min << max << std::endl; return true; \
    }

#define RADIALSHOOTER_VEC3_FUNCTION( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        osg::Vec3d min, max; is >> min >> max; \
        ud.add( "set" #PROP, min, max ); return true; \
    } \
    static bool write##PROP( OutputStream& os, OutputUserData& ud ) { \
        const IRCommand* cmd = ud.findCommand("set" #PROP); \
        osg::Vec3d min, max; \
        if (cmd && cmd->args.size() > 1) { \
            min = irArgVec3d(cmd->args[0]); max = irArgVec3d(cmd->args[1]); } \
        os << min << max << std::endl; return true; \
    }

RADIALSHOOTER_FLOAT_FUNCTION( ThetaRange )
RADIALSHOOTER_FLOAT_FUNCTION( PhiRange )
RADIALSHOOTER_FLOAT_FUNCTION( InitialSpeedRange )
RADIALSHOOTER_VEC3_FUNCTION( InitialRotationalSpeedRange )

REGISTER_OBJECT_WRAPPER( osgParticleRadialShooter,
                         new osgParticle::RadialShooter,
                         osgParticle::RadialShooter,
                         "osg::Object osgParticle::Shooter osgParticle::RadialShooter" )
{
    ADD_USER_SERIALIZER( ThetaRange );  // _theta_range
    ADD_USER_WRITER( ThetaRange );
    ADD_USER_SERIALIZER( PhiRange );  // _phi_range
    ADD_USER_WRITER( PhiRange );
    ADD_USER_SERIALIZER( InitialSpeedRange );  // _speed_range
    ADD_USER_WRITER( InitialSpeedRange );
    ADD_USER_SERIALIZER( InitialRotationalSpeedRange );  // _rot_speed_range
    ADD_USER_WRITER( InitialRotationalSpeedRange );
}
