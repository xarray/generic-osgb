#include <GenericReserializer.h>
using namespace osgVerse;

// _tileID
static bool readTileID(InputStream& is, InputUserData& ud)
{
    int level = 0, x = 0, y = 0;
    is >> level >> x >> y;
    ud.add( "setTileID", level, x, y );
    return true;
}

// _colorLayers
static bool readColorLayers(InputStream& is, InputUserData& ud)
{
    unsigned int numValidLayers = 0; is >> numValidLayers >> is.BEGIN_BRACKET;
    for ( unsigned int i = 0; i < numValidLayers; ++i )
    {
        unsigned int layerNum = 0; is >> is.PROPERTY("Layer") >> layerNum;
        ObjectTypeAndID layer = ud.readObjectFromStream( is, "osgTerrain::Layer" );
        if ( layer.valid() ) ud.add( "setColorLayer", layerNum, &layer );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_TerrainTile,
                         new osgTerrain::TerrainTile,
                         osgTerrain::TerrainTile,
                         "osg::Object osg::Node osg::Group osgTerrain::TerrainTile" )
{
    ADD_USER_SERIALIZER( TileID );  // _tileID
    ADD_OBJECT_SERIALIZER( TerrainTechnique, osgTerrain::TerrainTechnique, NULL );  // _terrainTechnique
    ADD_OBJECT_SERIALIZER( Locator, osgTerrain::Locator, NULL );  // _locator
    ADD_OBJECT_SERIALIZER( ElevationLayer, osgTerrain::Layer, NULL );  // _elevationLayer
    ADD_USER_SERIALIZER( ColorLayers );  // _colorLayers
    ADD_BOOL_SERIALIZER( RequiresNormals, true );  // _requiresNormals
    ADD_BOOL_SERIALIZER( TreatBoundariesToValidDataAsDefaultValue, false );  // _treatBoundariesToValidDataAsDefaultValue
    BEGIN_ENUM_SERIALIZER( BlendingPolicy, INHERIT );
        ADD_ENUM_VALUE( INHERIT );
        ADD_ENUM_VALUE( DO_NOT_SET_BLENDING );
        ADD_ENUM_VALUE( ENABLE_BLENDING );
        ADD_ENUM_VALUE( ENABLE_BLENDING_WHEN_ALPHA_PRESENT );
    END_ENUM_SERIALIZER();  // BlendingPolicy
}
