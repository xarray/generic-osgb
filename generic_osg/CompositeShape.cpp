#include <GenericReserializer.h>
using namespace osgVerse;

static bool readChildren(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID child = is.readObject(ud);
        if (child.valid()) ud.add("addChild", &child);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeChildren(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addChild", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.empty() ) continue;
        os.writeObject(ud, ud.resolve(c.args[0]));
        if (!os.isBinary()) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( CompositeShape,
                         new osg::CompositeShape,
                         osg::CompositeShape,
                         "osg::Object osg::Shape osg::CompositeShape" )
{
    ADD_OBJECT_SERIALIZER( Shape, osg::Shape, NULL );  // _shape
    ADD_USER_SERIALIZER( Children );  //_children
    ADD_USER_WRITER( Children );
}
