#include <GenericReserializer.h>
using namespace osgVerse;

static bool readVertices(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::Vec3d vec; is >> vec;
        ud.add("addVertex",  vec );
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeVertices(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addVertex", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
        os << (cmds[i]->args.empty() ? osg::Vec3d() : irArgVec3d(cmds[i]->args[0]))
           << std::endl;
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleMultiSegmentPlacer,
                         new osgParticle::MultiSegmentPlacer,
                         osgParticle::MultiSegmentPlacer,
                         "osg::Object osgParticle::Placer osgParticle::MultiSegmentPlacer" )
{
    ADD_USER_SERIALIZER( Vertices );  // _vx
    ADD_USER_WRITER( Vertices );
}
