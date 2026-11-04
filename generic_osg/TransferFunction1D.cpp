#include <GenericReserializer.h>
using namespace osgVerse;

#if 1

static bool readColorMap(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float key = 0.0f;
        osg::Vec4d value;
        is >> key >> value;
        ud.add("setColor", key, value);
    }
    is >> is.END_BRACKET;
    return true;
}

#endif


static bool writeColorMap(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setColor", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 2 ) continue;
        os << (float)irArgDouble(c.args[0]) << irArgVec4d(c.args[1]) << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( TransferFunction1D,
                         new osg::TransferFunction1D,
                         osg::TransferFunction1D,
                         "osg::Object osg::TransferFunction osg::TransferFunction1D" )
{
#if 1
    ADD_USER_SERIALIZER( ColorMap );  // _colorMap
    ADD_USER_WRITER( ColorMap );
#else
    ADD_MAP_SERIALIZER(ColorMap, osg::TransferFunction1D::ColorMap, osgDB::BaseSerializer::RW_FLOAT, osgDB::BaseSerializer::RW_VEC4F);
#endif
}
