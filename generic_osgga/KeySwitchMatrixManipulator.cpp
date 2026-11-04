#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

static bool readKeyManipMap(InputStream& is, InputUserData& ud)
{
    int activeCameraManipulatorIndex = -1; is>>activeCameraManipulatorIndex;
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        int key; is >> key;
        std::string name; is >> name;
        
        ObjectTypeAndID obj = is.readObject(ud);
        if ( obj.valid() ) ud.add("addMatrixManipulator", key, name, &obj);
    }

    if (activeCameraManipulatorIndex>=0) ud.add("selectMatrixManipulator", activeCameraManipulatorIndex);
    
    return true;
}

static bool writeKeyManipMap(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addMatrixManipulator", cmds);

    int activeIndex = -1;
    const IRCommand* sel = ud.findCommand("selectMatrixManipulator");
    if (sel && !sel->args.empty()) activeIndex = (int)irArgInt(sel->args[0]);

    os << activeIndex;
    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if (c.args.size() < 3) continue;
        os << (int)irArgInt(c.args[0]);
        os << irArgString(c.args[1]);
        os.writeObject(ud, ud.resolve(c.args[2]));
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgGA_KeySwitchMatrixManipulator,
                         new osgGA::KeySwitchMatrixManipulator,
                         osgGA::KeySwitchMatrixManipulator,
                         "osg::Object osgGA::KeySwitchMatrixManipulator" )
{
    ADD_USER_SERIALIZER( KeyManipMap );  // _children
    ADD_USER_WRITER( KeyManipMap );
}
