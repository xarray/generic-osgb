#include <GenericReserializer.h>
using namespace osgVerse;

static bool readDomains(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string typeName;
        is >> is.PROPERTY("Domain") >> typeName >> is.BEGIN_BRACKET;

        osg::Plane plane;
        osg::Vec3d v1, v2, v3, s1, s2;
        double r1 = 0.0, r2 = 0.0;
        is >> is.PROPERTY("Plane") >> plane;
        is >> is.PROPERTY("Vertices1") >> v1;
        is >> is.PROPERTY("Vertices2") >> v2;
        is >> is.PROPERTY("Vertices3") >> v3;
        is >> is.PROPERTY("Basis1") >> s1;
        is >> is.PROPERTY("Basis2") >> s2;
        is >> is.PROPERTY("Factors") >> r1 >> r2;
        ud.add("addDomain", typeName, plane, v1, v2, v3, s1, s2, r1, r2);

        is >> is.END_BRACKET;
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeDomains(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addDomain", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 9 ) continue;

        os << os.PROPERTY("Domain") << irArgString(c.args[0]) << os.BEGIN_BRACKET << std::endl;
        os << os.PROPERTY("Plane")
           << osg::Plane(c.args[1].v[0], c.args[1].v[1], c.args[1].v[2], c.args[1].v[3])
           << std::endl;
        os << os.PROPERTY("Vertices1") << irArgVec3d(c.args[2]) << std::endl;
        os << os.PROPERTY("Vertices2") << irArgVec3d(c.args[3]) << std::endl;
        os << os.PROPERTY("Vertices3") << irArgVec3d(c.args[4]) << std::endl;
        os << os.PROPERTY("Basis1") << irArgVec3d(c.args[5]) << std::endl;
        os << os.PROPERTY("Basis2") << irArgVec3d(c.args[6]) << std::endl;
        os << os.PROPERTY("Factors") << irArgDouble(c.args[7]) << irArgDouble(c.args[8])
           << std::endl;
        os << os.END_BRACKET << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleDomainOperator,
                         new osgParticle::DomainOperator,
                         osgParticle::DomainOperator,
                         "osg::Object osgParticle::Operator osgParticle::DomainOperator" )
{
    ADD_USER_SERIALIZER( Domains );  // _placers
    ADD_USER_WRITER( Domains );
}
