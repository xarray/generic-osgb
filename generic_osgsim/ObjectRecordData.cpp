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

static bool writeData(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setData");
    unsigned int flags = 0, relativePriority = 0, effectID1 = 0, effectID2 = 0, significance = 0;
    float transparency = 0.0f;
    if (cmd && cmd->args.size() > 5)
    {
        flags = (unsigned int)irArgInt(cmd->args[0]);
        relativePriority = (unsigned int)irArgInt(cmd->args[1]);
        transparency = (float)irArgDouble(cmd->args[2]);
        effectID1 = (unsigned int)irArgInt(cmd->args[3]);
        effectID2 = (unsigned int)irArgInt(cmd->args[4]);
        significance = (unsigned int)irArgInt(cmd->args[5]);
    }

    os << os.PROPERTY("Flags") << flags << std::endl;
    os << os.PROPERTY("RelativePriority") << relativePriority << std::endl;
    os << os.PROPERTY("Transparency") << transparency << std::endl;
    os << os.PROPERTY("EffectID1") << effectID1 << std::endl;
    os << os.PROPERTY("EffectID2") << effectID2 << std::endl;
    os << os.PROPERTY("Significance") << significance << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ObjectRecordData,
                         new osgSim::ObjectRecordData,
                         osgSim::ObjectRecordData,
                         "osg::Object osgSim::ObjectRecordData" )
{
    ADD_USER_SERIALIZER( Data );
    ADD_USER_WRITER( Data );
}
