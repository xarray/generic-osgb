#include <GenericReserializer.h>
using namespace osgVerse;

#define BOXPLACER_FUNCTION( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        float min = 0.0f, max = 0.0f; is >> min >> max; \
        ud.add( "set" #PROP, min, max ); return true; \
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
    ADD_USER_SERIALIZER( YRange );  // _y_range
    ADD_USER_SERIALIZER( ZRange );  // _z_range
}
