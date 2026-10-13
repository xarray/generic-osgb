#include <GenericReserializer.h>
using namespace osgVerse;

static bool readOperators(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID op = ud.readObjectFromStream(is, "osgParticle::Operator");
        if ( op ) ud.add("addOperator",  op.get() );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgParticleModularProgram,
                         new osgParticle::ModularProgram,
                         osgParticle::ModularProgram,
                         "osg::Object osg::Node osgParticle::ParticleProcessor osgParticle::Program osgParticle::ModularProgram" )
{
    ADD_USER_SERIALIZER( Operators );  // _operators
}
