#include <GenericReserializer.h>
using namespace osgVerse;

#define BOXPLACER_FUNCTION( PROP ) \
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

BOXPLACER_FUNCTION( XRange )
BOXPLACER_FUNCTION( YRange )
BOXPLACER_FUNCTION( ZRange )

REGISTER_OBJECT_WRAPPER( osgParticleBoxPlacer,
                         new osgParticle::BoxPlacer,
                         osgParticle::BoxPlacer,
                         "osg::Object osgParticle::Placer osgParticle::CenteredPlacer osgParticle::BoxPlacer" )
{
    ADD_USER_SERIALIZER( XRange );  // _x_range
    ADD_USER_WRITER( XRange );
    ADD_USER_SERIALIZER( YRange );  // _y_range
    ADD_USER_WRITER( YRange );
    ADD_USER_SERIALIZER( ZRange );  // _z_range
    ADD_USER_WRITER( ZRange );
}
