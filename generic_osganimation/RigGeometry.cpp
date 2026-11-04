#include <algorithm>
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

static bool writeInfluenceMap(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setVertexInfluence", cmds);

    // Preserve the bone order of first appearance.
    std::vector<std::string> bones;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        if (cmds[i]->args.empty()) continue;
        std::string bone = irArgString(cmds[i]->args[0]);
        if (std::find(bones.begin(), bones.end(), bone) == bones.end()) bones.push_back(bone);
    }

    os << (unsigned int)bones.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t b=0; b<bones.size(); ++b )
    {
        os << os.PROPERTY("VertexInfluence");
        os.writeWrappedString(bones[b]);
        os << std::endl;

        std::vector<const IRCommand*> influences;
        for ( size_t i=0; i<cmds.size(); ++i )
        {
            if (!cmds[i]->args.empty() && irArgString(cmds[i]->args[0]) == bones[b])
                influences.push_back(cmds[i]);
        }

        os << (unsigned int)influences.size() << os.BEGIN_BRACKET << std::endl;
        for ( size_t i=0; i<influences.size(); ++i )
        {
            const IRCommand& c = *influences[i];
            int index = c.args.size() > 1 ? (int)irArgInt(c.args[1]) : 0;
            float weight = c.args.size() > 2 ? (float)irArgDouble(c.args[2]) : 0.0f;
            os << index << weight << std::endl;
        }
        os << os.END_BRACKET << std::endl;
    }
    os << os.END_BRACKET << std::endl;
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
    ADD_USER_WRITER( InfluenceMap );
    ADD_OBJECT_SERIALIZER( SourceGeometry, osg::Geometry, NULL );  // _geometry

    {
        UPDATE_TO_VERSION_SCOPED( 145 )
        ADD_OBJECT_SERIALIZER( RigTransformImplementation, osgAnimation::RigTransform, NULL );  // _geometry
    }

}
}
