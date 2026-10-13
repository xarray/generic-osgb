#include <GenericReserializer.h>
using namespace osgVerse;

static bool readMorphTargets(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float weight = 0.0f;
        is >> is.PROPERTY("MorphTarget") >> weight;
        ObjectTypeAndID target = ud.readObjectFromStream(is, "osg::Geometry");
        if ( target ) ud.add("addMorphTarget",  target.get(), weight );
    }
    is >> is.END_BRACKET;
    return true;
}

#define ADD_ARRAYDATA_FUNCTIONS( ORIGINAL_PROP, PROP ) \
    static bool read##ORIGINAL_PROP( InputStream& is, InputUserData& ud ) { \
        is >> is.BEGIN_BRACKET; \
        ObjectTypeAndID array = is.readArray(ud); \
        if ( array.valid() ) ud.add( "set" #PROP, &array ); \
        is >> is.END_BRACKET; \
        return true; \
    }
ADD_ARRAYDATA_FUNCTIONS( VertexData, VertexSource )
ADD_ARRAYDATA_FUNCTIONS( NormalData, NormalSource )

REGISTER_OBJECT_WRAPPER( osgAnimation_MorphGeometry,
                         new osgAnimation::MorphGeometry,
                         osgAnimation::MorphGeometry,
                         "osg::Object osg::Node osg::Drawable osg::Geometry osgAnimation::MorphGeometry" )
{
    BEGIN_ENUM_SERIALIZER( Method, NORMALIZED );
        ADD_ENUM_VALUE( NORMALIZED );
        ADD_ENUM_VALUE( RELATIVE );
    END_ENUM_SERIALIZER();  // _method

    ADD_USER_SERIALIZER( MorphTargets );  // _morphTargets
    ADD_BOOL_SERIALIZER( MorphNormals, true );  // _morphNormals
    ADD_USER_SERIALIZER( VertexData );  // VertexSource
    ADD_USER_SERIALIZER( NormalData );  // NormalSource

    {
            UPDATE_TO_VERSION_SCOPED( 147 )
            ADD_OBJECT_SERIALIZER( MorphTransformImplementation, osgAnimation::MorphTransform, NULL );  // _geometry
    }
}
