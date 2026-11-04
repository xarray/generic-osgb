#include <GenericReserializer.h>
using namespace osgVerse;

static bool readDraggers(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID child = ud.readObjectFromStream(is, "osgManipulator::Dragger");
        if ( child ) ud.add("addDragger",  child );
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeDraggers(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addDragger", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.empty() ) continue;
        int target = ud.resolve(c.args[0]);
        if ( os.isBinary() ) os << (target >= 0);
        os.writeObject(ud, target);
        if ( !os.isBinary() ) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgManipulator_CompositeDragger,
                         /*new osgManipulator::CompositeDragger*/NULL,
                         osgManipulator::CompositeDragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger "
                         "osgManipulator::CompositeDragger" )
{
    ADD_USER_SERIALIZER( Draggers );  // _draggerList
    ADD_USER_WRITER( Draggers );
}
