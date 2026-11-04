#include <GenericReserializer.h>
using namespace osgVerse;

static bool readImages(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID image = is.readObject(ud);
        if ( image.valid() ) ud.add("setImage", i, &image);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeImages(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setImage", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        int target = (c.args.size() > 1) ? ud.resolve(c.args[1]) : -1;
        os.writeObject(ud, target);
        if (!os.isBinary()) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Texture2DArray,
                         new osg::Texture2DArray,
                         osg::Texture2DArray,
                         "osg::Object osg::StateAttribute osg::Texture osg::Texture2DArray" )
{
    ADD_USER_SERIALIZER( Images );  // _images
    ADD_USER_WRITER( Images );
    ADD_INT_SERIALIZER( TextureWidth, 0 );  // _textureWidth
    ADD_INT_SERIALIZER( TextureHeight, 0 );  // _textureHeight
    ADD_INT_SERIALIZER( TextureDepth, 0 );  // _textureDepth
}
