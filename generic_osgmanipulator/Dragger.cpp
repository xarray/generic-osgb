#include <GenericReserializer.h>
using namespace osgVerse;

// TransformUpdating

static bool readTransformUpdating(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string name; is >> name >> is.BEGIN_BRACKET;
        if ( name=="DraggerTransformCallback" )
        {
            ObjectTypeAndID transform = ud.readObjectFromStream(is, "osg::MatrixTransform");
            if ( transform ) ud.add("addTransformUpdating",  transform.get() );
        }
        is >> is.END_BRACKET;
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeTransformUpdating(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addTransformUpdating", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.empty() ) continue;
        int target = ud.resolve(c.args[0]);

        os << std::string("DraggerTransformCallback") << os.BEGIN_BRACKET << std::endl;
        if ( os.isBinary() ) os << (target >= 0);
        os.writeObject(ud, target);
        if ( !os.isBinary() ) os << std::endl;
        os << os.END_BRACKET << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

// DefaultGeometry: FIXME - add a setUseDefaultGeometry(bool) here?

static bool readDefaultGeometry(InputStream& is, InputUserData& ud)
{
    bool useDefGeometry = false; is >> useDefGeometry;
    ud.add("setupDefaultGeometry", useDefGeometry);
    return true;
}

static bool writeDefaultGeometry(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setupDefaultGeometry");
    os << (bool)((cmd && !cmd->args.empty()) ? irArgBool(cmd->args[0]) : false) << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgManipulator_Dragger,
                         /*new osgManipulator::Dragger*/NULL,
                         osgManipulator::Dragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger" )
{
    // Dragger should not record children separately, so ignore the osg::Group class wrapper

    ADD_BOOL_SERIALIZER( HandleEvents, false );  // _handleEvents
    ADD_BOOL_SERIALIZER( DraggerActive, false );  // _draggerActive
    ADD_UINT_SERIALIZER( ActivationModKeyMask, 0 );  // _activationModKeyMask
    ADD_INT_SERIALIZER( ActivationKeyEvent, 0 );  // _activationKeyEvent
    ADD_USER_SERIALIZER( TransformUpdating );  // _draggerCallbacks
    ADD_USER_WRITER( TransformUpdating );
    ADD_USER_SERIALIZER( DefaultGeometry );
    ADD_USER_WRITER( DefaultGeometry );
}
