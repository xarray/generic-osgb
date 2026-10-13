#include <GenericReserializer.h>
using namespace osgVerse;

namespace wrap_osgAnimationRigGeometry{

static bool readInfluenceMap(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string bonename;
        is >> is.PROPERTY("VertexInfluence");
        is.readWrappedString(bonename);
        unsigned int viSize = is.readSize(); is >> is.BEGIN_BRACKET;
        for ( unsigned int j=0; j<viSize; ++j )
        {
            int index = 0;
            float weight = 0.0f;
            is >> index >> weight;
            ud.add("setVertexInfluence", bonename, index, weight);
        }
        is >> is.END_BRACKET;
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgAnimation_RigGeometry,
                         new osgAnimation::RigGeometry,
                         osgAnimation::RigGeometry,
                         "osg::Object osg::Node osg::Drawable osg::Geometry osgAnimation::RigGeometry" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::Node")
    }

    ADD_USER_SERIALIZER( InfluenceMap );  // _vertexInfluenceMap
    ADD_OBJECT_SERIALIZER( SourceGeometry, osg::Geometry, NULL );  // _geometry

    {
        UPDATE_TO_VERSION_SCOPED( 145 )
        ADD_OBJECT_SERIALIZER( RigTransformImplementation, osgAnimation::RigTransform, NULL );  // _geometry
    }

}
}
