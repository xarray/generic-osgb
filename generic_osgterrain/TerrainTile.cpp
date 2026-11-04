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

static bool writeTileID(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setTileID");
    int level = 0, x = 0, y = 0;
    if (cmd && cmd->args.size() > 2)
    {
        level = (int)irArgInt(cmd->args[0]); x = (int)irArgInt(cmd->args[1]);
        y = (int)irArgInt(cmd->args[2]);
    }
    os << level << x << y << std::endl;
    return true;
}

static bool writeColorLayers(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setColorLayer", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 2 ) continue;
        int target = ud.resolve(c.args[1]);

        os << os.PROPERTY("Layer") << (unsigned int)irArgInt(c.args[0]);
        if ( os.isBinary() ) os << (target >= 0);
        else os << std::endl;
        os.writeObject(ud, target);
        if ( !os.isBinary() ) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_TerrainTile,
                         new osgTerrain::TerrainTile,
                         osgTerrain::TerrainTile,
                         "osg::Object osg::Node osg::Group osgTerrain::TerrainTile" )
{
    ADD_USER_SERIALIZER( TileID );  // _tileID
    ADD_USER_WRITER( TileID );
    ADD_OBJECT_SERIALIZER( TerrainTechnique, osgTerrain::TerrainTechnique, NULL );  // _terrainTechnique
    ADD_OBJECT_SERIALIZER( Locator, osgTerrain::Locator, NULL );  // _locator
    ADD_OBJECT_SERIALIZER( ElevationLayer, osgTerrain::Layer, NULL );  // _elevationLayer
    ADD_USER_SERIALIZER( ColorLayers );  // _colorLayers
    ADD_USER_WRITER( ColorLayers );
    ADD_BOOL_SERIALIZER( RequiresNormals, true );  // _requiresNormals
    ADD_BOOL_SERIALIZER( TreatBoundariesToValidDataAsDefaultValue, false );  // _treatBoundariesToValidDataAsDefaultValue
    BEGIN_ENUM_SERIALIZER( BlendingPolicy, INHERIT );
        ADD_ENUM_VALUE2( INHERIT, 0 );
        ADD_ENUM_VALUE2( DO_NOT_SET_BLENDING, 1 );
        ADD_ENUM_VALUE2( ENABLE_BLENDING, 2 );
        ADD_ENUM_VALUE2( ENABLE_BLENDING_WHEN_ALPHA_PRESENT, 3 );
    END_ENUM_SERIALIZER();  // BlendingPolicy
}
