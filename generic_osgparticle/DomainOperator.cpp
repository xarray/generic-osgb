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

REGISTER_OBJECT_WRAPPER( osgParticleDomainOperator,
                         new osgParticle::DomainOperator,
                         osgParticle::DomainOperator,
                         "osg::Object osgParticle::Operator osgParticle::DomainOperator" )
{
    ADD_USER_SERIALIZER( Domains );  // _placers
}
