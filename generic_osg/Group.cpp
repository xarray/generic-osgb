#include <GenericReserializer.h>
using namespace osgVerse;

static bool readChildren(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        // Array elements are direct objects (starting with className); no hasObject flag
        ObjectTypeAndID obj = is.readObject(ud);
        if (obj.valid()) ud.add("addChild", &obj);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeChildren(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addChild", cmds);

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

REGISTER_OBJECT_WRAPPER( Group,
                         new osg::Group,
                         osg::Group,
                         "osg::Object osg::Node osg::Group" )
{
    ADD_USER_SERIALIZER( Children );  // _children
    ADD_USER_WRITER( Children );
}
