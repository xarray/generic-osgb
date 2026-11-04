#include <GenericReserializer.h>
using namespace osgVerse;

static bool readDrawables(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID drawable = is.readObject(ud);
        if (drawable.valid()) ud.add("addDrawable", &drawable);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeDrawables(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addDrawable", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        int index = cmds[i]->args.empty() ? -1 : ud.resolve(cmds[i]->args[0]);
        os.writeObject(ud, index);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( Geode,
                         new osg::Geode,
                         osg::Geode,
                         "osg::Object osg::Node osg::Geode" )
{
    ADD_USER_SERIALIZER( Drawables );  // _drawables
    ADD_USER_WRITER( Drawables );
}
