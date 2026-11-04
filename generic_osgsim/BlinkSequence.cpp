#include <GenericReserializer.h>
using namespace osgVerse;

static bool readPulseData(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        double length = 0.0;
        osg::Vec4 color;
        is >> length >> color;
        ud.add("addPulse",  length, color );
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writePulseData(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addPulse", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 2 ) continue;
        os << irArgDouble(c.args[0]) << irArgVec4f(c.args[1]) << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_BlinkSequence,
                         new osgSim::BlinkSequence,
                         osgSim::BlinkSequence,
                         "osg::Object osgSim::BlinkSequence" )
{
    ADD_DOUBLE_SERIALIZER( PhaseShift, 0.0 );  // _phaseShift
    ADD_USER_SERIALIZER( PulseData );  // _pulseData
    ADD_USER_WRITER( PulseData );
    ADD_OBJECT_SERIALIZER( SequenceGroup, osgSim::SequenceGroup, NULL );  // _sequenceGroup
}
