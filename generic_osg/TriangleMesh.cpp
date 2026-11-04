#include <GenericReserializer.h>
using namespace osgVerse;

#define ARRAY_FUNCTIONS( PROP, TYPE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        ObjectTypeAndID array = ud.readObjectFromStream(is, #TYPE); \
        ud.add("set" #PROP, &array); \
        return true; \
    } \
    static bool has##PROP(OutputUserData& ud) { \
        const IRArg* a = ud.findPropertyArg(#PROP); \
        return a && ud.resolve(*a) >= 0; \
    } \
    static bool write##PROP(OutputStream& os, OutputUserData& ud) { \
        const IRArg* a = ud.findPropertyArg(#PROP); \
        int target = a ? ud.resolve(*a) : -1; \
        if (os.isBinary()) os << (target >= 0); \
        if (target >= 0) os.writeObject(ud, target); \
        if (!os.isBinary()) os << std::endl; \
        return true; \
    }

ARRAY_FUNCTIONS( Vertices, osg::Vec3Array )
ARRAY_FUNCTIONS( Indices, osg::IndexArray )

REGISTER_OBJECT_WRAPPER( TriangleMesh,
                         new osg::TriangleMesh,
                         osg::TriangleMesh,
                         "osg::Object osg::Shape osg::TriangleMesh" )
{
    ADD_USER_SERIALIZER( Vertices );  // _vertices
    ADD_USER_WRITER_IF( Vertices, hasVertices );
    ADD_USER_SERIALIZER( Indices );  // _indices
    ADD_USER_WRITER_IF( Indices, hasIndices );
}
