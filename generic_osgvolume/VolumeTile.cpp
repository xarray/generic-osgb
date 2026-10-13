#include <GenericReserializer.h>
using namespace osgVerse;

static bool readTileID(InputStream& is, InputUserData& ud)
{
    int level = 0, x = 0, y = 0, z = 0;
    is >> level >> x >> y >> z;
    ud.add("setTileID", level, x, y, z);
    return true;
}

REGISTER_OBJECT_WRAPPER( osgVolume_VolumeTile,
                         new osgVolume::VolumeTile,
                         osgVolume::VolumeTile,
                         "osg::Object osg::Node osg::Group osgVolume::VolumeTile" )
{
    ADD_OBJECT_SERIALIZER( Volume, osgVolume::Volume, NULL );  // _volume
    {
        UPDATE_TO_VERSION_SCOPED( 90 )
        REMOVE_SERIALIZER( Volume );
    }

    ADD_BOOL_SERIALIZER( Dirty, false );  // _dirty
    ADD_USER_SERIALIZER( TileID );  // _tileID
    ADD_OBJECT_SERIALIZER( VolumeTechnique, osgVolume::VolumeTechnique, NULL );  // _volumeTechnique
    ADD_OBJECT_SERIALIZER( Locator, osgVolume::Locator, NULL );  // _locator
    ADD_OBJECT_SERIALIZER( Layer, osgVolume::Layer, NULL );  // _layer
}
