#include <GenericReserializer.h>
using namespace osgVerse;

#define SECTORPLACER_FUNCTION( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        float min = 0.0f, max = 0.0f; is >> min >> max; \
        ud.add( "set" #PROP, min, max ); return true; \
    }

SECTORPLACER_FUNCTION( RadiusRange )
SECTORPLACER_FUNCTION( PhiRange )

REGISTER_OBJECT_WRAPPER( osgParticleSectorPlacer,
                         new osgParticle::SectorPlacer,
                         osgParticle::SectorPlacer,
                         "osg::Object osgParticle::Placer osgParticle::CenteredPlacer osgParticle::SectorPlacer" )
{
    ADD_USER_SERIALIZER( RadiusRange );  // _rad_range
    ADD_USER_SERIALIZER( PhiRange );  // _phi_range
}
