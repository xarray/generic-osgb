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

static bool hasMorphTargets(OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addMorphTarget", cmds);
    return !cmds.empty();
}

#define ADD_ARRAYDATA_FUNCTIONS( ORIGINAL_PROP, PROP ) \
    static bool read##ORIGINAL_PROP( InputStream& is, InputUserData& ud ) { \
        is >> is.BEGIN_BRACKET; \
        ObjectTypeAndID array = is.readArray(ud); \
        if ( array.valid() ) ud.add( "set" #PROP, &array ); \
        is >> is.END_BRACKET; \
        return true; \
    } \
    static bool has##ORIGINAL_PROP( OutputUserData& ud ) { \
        const IRCommand* cmd = ud.findCommand( "set" #PROP ); \
        return cmd && !cmd->args.empty() && ud.resolve( cmd->args[0] ) >= 0; \
    } \
    static bool write##ORIGINAL_PROP( OutputStream& os, OutputUserData& ud ) { \
        const IRCommand* cmd = ud.findCommand("set" #PROP); \
        int target = (cmd && !cmd->args.empty()) ? ud.resolve(cmd->args[0]) : -1; \
        os << os.BEGIN_BRACKET << std::endl; \
        if ( target >= 0 ) os.writeCompactArray(ud, target); \
        os << os.END_BRACKET << std::endl; \
        return true; \
    }
ADD_ARRAYDATA_FUNCTIONS( VertexData, VertexSource )
ADD_ARRAYDATA_FUNCTIONS( NormalData, NormalSource )

static bool writeMorphTargets(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addMorphTarget", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 2 ) continue;
        int target = ud.resolve(c.args[0]);

        os << os.PROPERTY("MorphTarget") << (float)irArgDouble(c.args[1]);
        if ( os.isBinary() ) os << (target >= 0);
        else os << std::endl;
        os.writeObject(ud, target);
        if ( !os.isBinary() ) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgAnimation_MorphGeometry,
                         new osgAnimation::MorphGeometry,
                         osgAnimation::MorphGeometry,
                         "osg::Object osg::Node osg::Drawable osg::Geometry osgAnimation::MorphGeometry" )
{
    BEGIN_ENUM_SERIALIZER( Method, NORMALIZED );
        ADD_ENUM_VALUE2( NORMALIZED, 0 );
        ADD_ENUM_VALUE2( RELATIVE, 1 );
    END_ENUM_SERIALIZER();  // _method

    ADD_USER_SERIALIZER( MorphTargets );  // _morphTargets
    ADD_USER_WRITER_IF( MorphTargets, hasMorphTargets );
    ADD_BOOL_SERIALIZER( MorphNormals, true );  // _morphNormals
    ADD_USER_SERIALIZER( VertexData );  // VertexSource
    ADD_USER_WRITER_IF( VertexData, hasVertexData );
    ADD_USER_SERIALIZER( NormalData );  // NormalSource
    ADD_USER_WRITER_IF( NormalData, hasNormalData );

    {
            UPDATE_TO_VERSION_SCOPED( 147 )
            ADD_OBJECT_SERIALIZER( MorphTransformImplementation, osgAnimation::MorphTransform, NULL );  // _geometry
    }
}
