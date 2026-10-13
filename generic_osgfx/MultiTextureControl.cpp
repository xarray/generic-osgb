#include <GenericReserializer.h>
using namespace osgVerse;

static bool readTextureWeights(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float weight = 0.0f; is >> weight;
        ud.add("setTextureWeight",  i, weight );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgFX_MultiTextureControl,
                         new osgFX::MultiTextureControl,
                         osgFX::MultiTextureControl,
                         "osg::Object osg::Node osg::Group osgFX::MultiTextureControl" )
{
    ADD_USER_SERIALIZER( TextureWeights );  // _textureWeightList

    {
        UPDATE_TO_VERSION_SCOPED( 116 )

        REMOVE_SERIALIZER( TextureWeights );  // _textureWeightList

        ADD_OBJECT_SERIALIZER( TextureWeights, osgFX::MultiTextureControl::TextureWeights, NULL );
        ADD_BOOL_SERIALIZER( UseTexEnvCombine, true );
        ADD_BOOL_SERIALIZER( UseTextureWeightsUniform, true );
    }
}
