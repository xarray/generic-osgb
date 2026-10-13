#include <GenericReserializer.h>
using namespace osgVerse;

extern bool readParticle( InputStream& is, InputUserData& ud );

// _def_bbox

static bool readDefaultBoundingBox(InputStream& is, InputUserData& ud)
{
    osg::Vec3d min, max;
    is >> is.BEGIN_BRACKET;
    is >> is.PROPERTY("Minimum") >> min;
    is >> is.PROPERTY("Maximum") >> max;
    is >> is.END_BRACKET;
    ud.add("setDefaultBoundingBox",  osg::BoundingBox(min, max) );
    return true;
}

// _defaultParticleTemplate

static bool readDefaultParticleTemplate(InputStream& is, InputUserData& ud)
{
    ud.add("setDefaultParticleTemplate");
    readParticle( is, ud );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleParticleSystem,
                         new osgParticle::ParticleSystem,
                         osgParticle::ParticleSystem,
                         "osg::Object osg::Drawable osgParticle::ParticleSystem" )
{
    ADD_USER_SERIALIZER( DefaultBoundingBox );  // _def_bbox

    BEGIN_ENUM_SERIALIZER2( ParticleAlignment, osgParticle::ParticleSystem::Alignment, BILLBOARD );
        ADD_ENUM_VALUE( BILLBOARD );
        ADD_ENUM_VALUE( FIXED );
    END_ENUM_SERIALIZER();  // _alignment

    ADD_VEC3_SERIALIZER( AlignVectorX, osg::Vec3() );  // _align_X_axis
    ADD_VEC3_SERIALIZER( AlignVectorY, osg::Vec3() );  // _align_Y_axis

    BEGIN_ENUM_SERIALIZER( ParticleScaleReferenceFrame, WORLD_COORDINATES );
        ADD_ENUM_VALUE( LOCAL_COORDINATES );
        ADD_ENUM_VALUE( WORLD_COORDINATES );
    END_ENUM_SERIALIZER();  // _particleScaleReferenceFrame

    ADD_BOOL_SERIALIZER( UseVertexArray, false );  // _useVertexArray
    ADD_BOOL_SERIALIZER( UseShaders, false );  // _useShaders
    ADD_BOOL_SERIALIZER( DoublePassRendering, false );  // _doublepass
    ADD_BOOL_SERIALIZER( Frozen, false );  // _frozen
    ADD_USER_SERIALIZER( DefaultParticleTemplate );  // _def_ptemp
    ADD_BOOL_SERIALIZER( FreezeOnCull, false );  // _freeze_on_cull

    BEGIN_ENUM_SERIALIZER( SortMode, NO_SORT );
        ADD_ENUM_VALUE( NO_SORT );
        ADD_ENUM_VALUE( SORT_FRONT_TO_BACK );
        ADD_ENUM_VALUE( SORT_BACK_TO_FRONT );
    END_ENUM_SERIALIZER();  // _sortMode

    ADD_DOUBLE_SERIALIZER( VisibilityDistance, -1.0 );  // _visibilityDistance
}
