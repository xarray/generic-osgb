#include <GenericReserializer.h>
using namespace osgVerse;

static bool readUDC_UserData(InputStream& is, InputUserData& ud)
{
    is >> is.BEGIN_BRACKET;
    ObjectTypeAndID object = ud.readObjectFromStream(is, "osg::Object");
    if(object.valid()) ud.add("setUserData", &object);
    is >> is.END_BRACKET;
    return true;
}

// _descriptions
static bool readUDC_Descriptions(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string value;
        is.readWrappedString( value );
        ud.add("addDescription", value);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool readUDC_UserObjects(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID object = is.readObject(ud);
        if (object.valid()) ud.add("addUserObject", &object);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool hasUDC_UserData(OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArg("UDC_UserData");
    return a && ud.resolve(*a) >= 0;
}

static bool writeUDC_UserData(OutputStream& os, OutputUserData& ud)
{
    const IRArg* a = ud.findPropertyArg("UDC_UserData");
    int target = a ? ud.resolve(*a) : -1;

    // readUDC_UserData() expects a bracket block; in binary the embedded
    // readObjectFromStream() also consumes its own "has object" flag.
    os << os.BEGIN_BRACKET << std::endl;
    if (os.isBinary()) os << (target >= 0);
    os.writeObject(ud, target);
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool hasUDC_Descriptions(OutputUserData& ud)
{ return ud.countCommands("addDescription") > 0; }

static bool hasUDC_UserObjects(OutputUserData& ud)
{ return ud.countCommands("addUserObject") > 0; }

static bool writeUDC_Descriptions(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addDescription", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        os.writeWrappedString(cmds[i]->args.empty()
                              ? std::string() : irArgString(cmds[i]->args[0]));
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeUDC_UserObjects(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addUserObject", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        int target = cmds[i]->args.empty() ? -1 : ud.resolve(cmds[i]->args[0]);
        os.writeObject(ud, target);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

namespace UserDataContainerNamespace
{
    REGISTER_OBJECT_WRAPPER( UserDataContainer,
                            0,
                            osg::UserDataContainer,
                            "osg::Object osg::UserDataContainer" )
    {
    }
}

namespace DefaultUserDataContainerNamespace
{
    REGISTER_OBJECT_WRAPPER( DefaultUserDataContainer,
                            new osg::DefaultUserDataContainer,
                            osg::DefaultUserDataContainer,
                            "osg::Object osg::UserDataContainer osg::DefaultUserDataContainer" )
    {
        ADD_USER_SERIALIZER( UDC_UserData );  // _userData
        ADD_USER_WRITER_IF( UDC_UserData, &hasUDC_UserData );
        ADD_USER_SERIALIZER( UDC_Descriptions );  // _descriptions
        ADD_USER_WRITER_IF( UDC_Descriptions, &hasUDC_Descriptions );
        ADD_USER_SERIALIZER( UDC_UserObjects );  // _userData
        ADD_USER_WRITER_IF( UDC_UserObjects, &hasUDC_UserObjects );
    }
}
