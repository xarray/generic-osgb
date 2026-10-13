#include <GenericReserializer.h>
using namespace osgVerse;

static bool readData(InputStream& is, InputUserData& ud)
{
    unsigned int flags = 0, relativePriority = 0, effectID1 = 0, effectID2 = 0, significance = 0;
    float transparency = 0.0f;
    is >> is.PROPERTY("Flags") >> flags;
    is >> is.PROPERTY("RelativePriority") >> relativePriority;
    is >> is.PROPERTY("Transparency") >> transparency;
    is >> is.PROPERTY("EffectID1") >> effectID1;
    is >> is.PROPERTY("EffectID2") >> effectID2;
    is >> is.PROPERTY("Significance") >> significance;
    ud.add("setData", flags, relativePriority, transparency, effectID1, effectID2, significance);
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ObjectRecordData,
                         new osgSim::ObjectRecordData,
                         osgSim::ObjectRecordData,
                         "osg::Object osgSim::ObjectRecordData" )
{
    ADD_USER_SERIALIZER( Data );
}
