#include <GenericReserializer.h>
using namespace osgVerse;

extern bool readParticle( InputStream& is, InputUserData& ud );
extern bool writeParticle( OutputStream& os, OutputUserData& ud );

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

static bool hasDefaultBoundingBox(OutputUserData& ud)
{ return ud.findCommand("setDefaultBoundingBox") != NULL; }

static bool writeDefaultBoundingBox(OutputStream& os, OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArgOfKind("DefaultBoundingBox", IRArg::BBOX);
    osg::Vec3d min, max;
    if (a)
    {
        for (int i = 0; i < 3; ++i) { min._v[i] = a->v[i]; max._v[i] = a->v[i + 3]; }
    }

    os << os.BEGIN_BRACKET << std::endl;
    if (!os.isBinary()) os << os.PROPERTY("Minimum");
    os << min << std::endl;
    if (!os.isBinary()) os << os.PROPERTY("Maximum");
    os << max << std::endl;
    os << os.END_BRACKET << std::endl;
    return true;
}

// _defaultParticleTemplate

static bool readDefaultParticleTemplate(InputStream& is, InputUserData& ud)
{
    ud.add("setDefaultParticleTemplate");
    readParticle( is, ud );
    return true;
}

static bool writeDefaultParticleTemplate(OutputStream& os, OutputUserData& ud)
{ return writeParticle(os, ud); }

REGISTER_OBJECT_WRAPPER( osgParticleParticleSystem,
                         new osgParticle::ParticleSystem,
                         osgParticle::ParticleSystem,
                         "osg::Object osg::Node osg::Drawable osgParticle::ParticleSystem" )
{
    {
        // Since osg 154 the Drawable-derived classes also carry the Node fields
        // (StateSet, CullingActive, NodeMask) when written at a modern version.
        UPDATE_TO_VERSION_SCOPED( 154 )
        ADDED_ASSOCIATE("osg::Node")
    }

    ADD_USER_SERIALIZER( DefaultBoundingBox );  // _def_bbox
    ADD_USER_WRITER_IF( DefaultBoundingBox, hasDefaultBoundingBox );

    BEGIN_ENUM_SERIALIZER2( ParticleAlignment, osgParticle::ParticleSystem::Alignment, BILLBOARD );
        ADD_ENUM_VALUE2( BILLBOARD, 0 );
        ADD_ENUM_VALUE2( FIXED, 1 );
    END_ENUM_SERIALIZER();  // _alignment

    ADD_VEC3_SERIALIZER( AlignVectorX, osg::Vec3() );  // _align_X_axis
    ADD_VEC3_SERIALIZER( AlignVectorY, osg::Vec3() );  // _align_Y_axis

    BEGIN_ENUM_SERIALIZER( ParticleScaleReferenceFrame, WORLD_COORDINATES );
        ADD_ENUM_VALUE2( LOCAL_COORDINATES, 0 );
        ADD_ENUM_VALUE2( WORLD_COORDINATES, 1 );
    END_ENUM_SERIALIZER();  // _particleScaleReferenceFrame

    ADD_BOOL_SERIALIZER( UseVertexArray, false );  // _useVertexArray
    ADD_BOOL_SERIALIZER( UseShaders, false );  // _useShaders
    ADD_BOOL_SERIALIZER( DoublePassRendering, false );  // _doublepass
    ADD_BOOL_SERIALIZER( Frozen, false );  // _frozen
    ADD_USER_SERIALIZER( DefaultParticleTemplate );  // _def_ptemp
    ADD_USER_WRITER( DefaultParticleTemplate );
    ADD_BOOL_SERIALIZER( FreezeOnCull, false );  // _freeze_on_cull

    BEGIN_ENUM_SERIALIZER( SortMode, NO_SORT );
        ADD_ENUM_VALUE2( NO_SORT, 0 );
        ADD_ENUM_VALUE2( SORT_FRONT_TO_BACK, 1 );
        ADD_ENUM_VALUE2( SORT_BACK_TO_FRONT, 2 );
    END_ENUM_SERIALIZER();  // _sortMode

    ADD_DOUBLE_SERIALIZER( VisibilityDistance, -1.0 );  // _visibilityDistance
}
