// LibraryWrapper: the activation anchor for the osgTerrain wrappers.
//
// Each wrapper registers statically through REGISTER_OBJECT_WRAPPER, so a
// whole-archive build runs every registrar automatically. With
// ENABLE_STATIC_LIBRARY the executable activates this library through
// USE_SERIALIZER_WRAPPER_LIBRARY(osgTerrain) (see GenericStaticWrappers.h),
// which is what keeps the registrars below alive in a plain static link.
#include <GenericReserializer.h>
using namespace osgVerse;

USE_SERIALIZER_WRAPPER(osgTerrain_CompositeLayer)
USE_SERIALIZER_WRAPPER(osgTerrain_ContourLayer)
USE_SERIALIZER_WRAPPER(osgTerrain_DisplacementMappingTechnique)
USE_SERIALIZER_WRAPPER(osgTerrain_GeometryTechnique)
USE_SERIALIZER_WRAPPER(osgTerrain_HeightFieldLayer)
USE_SERIALIZER_WRAPPER(osgTerrain_ImageLayer)
USE_SERIALIZER_WRAPPER(osgTerrain_Layer)
USE_SERIALIZER_WRAPPER(osgTerrain_Locator)
USE_SERIALIZER_WRAPPER(osgTerrain_ProxyLayer)
USE_SERIALIZER_WRAPPER(osgTerrain_SwitchLayer)
USE_SERIALIZER_WRAPPER(osgTerrain_Terrain)
USE_SERIALIZER_WRAPPER(osgTerrain_TerrainTechnique)
USE_SERIALIZER_WRAPPER(osgTerrain_TerrainTile)

extern "C" void wrapper_serializer_library_osgTerrain(void) {}
