#include <GenericReserializer.h>
using namespace osgVerse;

static bool readArea(InputStream& is, InputUserData& ud)
{
    unsigned int x, y, w, h;
    is >> x >> y >> w >> h;
    ud.add("setSubImageDimensions", x, y, w, h);
    return true;
}

static bool writeArea(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setSubImageDimensions");
    unsigned int x = 0, y = 0, w = 0, h = 0;
    if (cmd && cmd->args.size() > 3)
    {
        x = (unsigned int)irArgInt(cmd->args[0]); y = (unsigned int)irArgInt(cmd->args[1]);
        w = (unsigned int)irArgInt(cmd->args[2]); h = (unsigned int)irArgInt(cmd->args[3]);
    }
    os << x << y << w << h << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( DrawPixels,
                         new osg::DrawPixels,
                         osg::DrawPixels,
                         "osg::Object osg::Node osg::Drawable osg::DrawPixels" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::Node")
    }

    ADD_VEC3_SERIALIZER( Position, osg::Vec3() );  // _position
    ADD_IMAGE_SERIALIZER( Image, osg::Image, NULL );  // _image
    ADD_BOOL_SERIALIZER( UseSubImage, false );  // _useSubImage
    ADD_USER_SERIALIZER( Area );  // _offsetX, _offsetY, _width, _height
    ADD_USER_WRITER( Area );
}
