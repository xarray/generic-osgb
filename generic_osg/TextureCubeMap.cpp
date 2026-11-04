#include <GenericReserializer.h>
using namespace osgVerse;

enum osg_TextureCubeMap_Face {
    POSITIVE_X = 0, NEGATIVE_X = 1,
    POSITIVE_Y = 2, NEGATIVE_Y = 3,
    POSITIVE_Z = 4, NEGATIVE_Z = 5
};

#define FACE_IMAGE_FUNCTION( PROP, FACE ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        bool hasImage; is >> hasImage; \
        if ( hasImage ) { \
            is >> is.BEGIN_BRACKET; ud.add( "setImage" #FACE, is.readImage(ud)); \
            is >> is.END_BRACKET; \
        } \
        return true; \
    } \
    static const IRCommand* find##PROP(OutputUserData& ud) \
    { return ud.findCommand("setImage" #FACE); } \
    static bool has##PROP(OutputUserData& ud) { \
        const IRCommand* cmd = find##PROP(ud); \
        return cmd && !cmd->args.empty() && ud.resolve(cmd->args[0]) >= 0; \
    } \
    static bool write##PROP(OutputStream& os, OutputUserData& ud) { \
        const IRCommand* cmd = find##PROP(ud); \
        int target = (cmd && !cmd->args.empty()) ? ud.resolve(cmd->args[0]) : -1; \
        os << (target >= 0); \
        if (target >= 0) { \
            os << os.BEGIN_BRACKET << std::endl; \
            os.writeImage(ud, target); \
            os << os.END_BRACKET << std::endl; \
        } \
        else if (!os.isBinary()) os << std::endl; \
        return true; \
    }

FACE_IMAGE_FUNCTION( PosX, osg_TextureCubeMap_Face::POSITIVE_X )
FACE_IMAGE_FUNCTION( NegX, osg_TextureCubeMap_Face::NEGATIVE_X )
FACE_IMAGE_FUNCTION( PosY, osg_TextureCubeMap_Face::POSITIVE_Y )
FACE_IMAGE_FUNCTION( NegY, osg_TextureCubeMap_Face::NEGATIVE_Y )
FACE_IMAGE_FUNCTION( PosZ, osg_TextureCubeMap_Face::POSITIVE_Z )
FACE_IMAGE_FUNCTION( NegZ, osg_TextureCubeMap_Face::NEGATIVE_Z )

REGISTER_OBJECT_WRAPPER( TextureCubeMap,
                         new osg::TextureCubeMap,
                         osg::TextureCubeMap,
                         "osg::Object osg::StateAttribute osg::Texture osg::TextureCubeMap" )
{
    ADD_USER_SERIALIZER( PosX );
    ADD_USER_WRITER_IF( PosX, hasPosX );
    ADD_USER_SERIALIZER( NegX );
    ADD_USER_WRITER_IF( NegX, hasNegX );
    ADD_USER_SERIALIZER( PosY );
    ADD_USER_WRITER_IF( PosY, hasPosY );
    ADD_USER_SERIALIZER( NegY );
    ADD_USER_WRITER_IF( NegY, hasNegY );
    ADD_USER_SERIALIZER( PosZ );
    ADD_USER_WRITER_IF( PosZ, hasPosZ );
    ADD_USER_SERIALIZER( NegZ );  // _images
    ADD_USER_WRITER_IF( NegZ, hasNegZ );

    ADD_INT_SERIALIZER( TextureWidth, 0 );  // _textureWidth
    ADD_INT_SERIALIZER( TextureHeight, 0 );  // _textureHeight
}
