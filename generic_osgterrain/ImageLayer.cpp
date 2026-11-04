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

static bool writeImage(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setImage");
    int target = (cmd && !cmd->args.empty()) ? ud.resolve(cmd->args[0]) : -1;

    if ( !os.isBinary() ) os << os.BEGIN_BRACKET << std::endl;
    if ( target >= 0 ) os.writeImage(ud, target);
    if ( !os.isBinary() ) os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_ImageLayer,
                         new osgTerrain::ImageLayer,
                         osgTerrain::ImageLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::ImageLayer" )
{
    ADD_USER_SERIALIZER( Image );
    ADD_USER_WRITER( Image );
}
