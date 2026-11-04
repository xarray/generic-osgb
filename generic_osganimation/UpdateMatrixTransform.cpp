#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

static bool readStackedTransforms(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID element = ud.readObjectFromStream(is, "osgAnimation::StackedTransformElement");
        if ( element ) ud.add("addStackedTransform", i, &element);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeStackedTransforms(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addStackedTransform", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if (c.args.size() < 2) continue;
        int target = ud.resolve(c.args[1]);
        if ( os.isBinary() ) os << (target >= 0);
        os.writeObject(ud, target);
        if ( !os.isBinary() ) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgAnimation_UpdateMatrixTransform,
                         new osgAnimation::UpdateMatrixTransform,
                         osgAnimation::UpdateMatrixTransform,
                         "osg::Object osg::Callback osg::NodeCallback osgAnimation::UpdateMatrixTransform" )
{
    ADD_USER_SERIALIZER( StackedTransforms );  // _transforms
    ADD_USER_WRITER( StackedTransforms );
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
