#include <GenericReserializer.h>
using namespace osgVerse;

// _image
static bool readImage(InputStream& is, InputUserData& ud)
{
    if ( !is.isBinary() ) is >> is.BEGIN_BRACKET;

    ObjectTypeAndID image = is.readImage( ud );
    if ( image.valid() ) ud.add( "setImage", &image );

    if ( !is.isBinary() ) is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_ImageLayer,
                         new osgTerrain::ImageLayer,
                         osgTerrain::ImageLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::ImageLayer" )
{
    ADD_USER_SERIALIZER( Image );
}
