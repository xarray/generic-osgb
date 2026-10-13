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

REGISTER_OBJECT_WRAPPER( osgSim_BlinkSequence,
                         new osgSim::BlinkSequence,
                         osgSim::BlinkSequence,
                         "osg::Object osgSim::BlinkSequence" )
{
    ADD_DOUBLE_SERIALIZER( PhaseShift, 0.0 );  // _phaseShift
    ADD_USER_SERIALIZER( PulseData );  // _pulseData
    ADD_OBJECT_SERIALIZER( SequenceGroup, osgSim::SequenceGroup, NULL );  // _sequenceGroup
}
