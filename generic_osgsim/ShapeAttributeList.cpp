#include <GenericReserializer.h>
using namespace osgVerse;

static bool readAttributes(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string name; int type = 0;
        is >> is.PROPERTY("ShapeAttribute") >> name;
        is >> is.PROPERTY("Type") >> type;
        switch ( type )
        {
        case 1:  // osgSim::ShapeAttribute::INTEGER
            {
                int value; is >> value;
                ud.add("addAttribute", name, value);
            }
            break;
        case 2:  // osgSim::ShapeAttribute::DOUBLE
            {
                double value; is >> value;
                ud.add("addAttribute", name, value);
            }
            break;
        default:  // STRING
            {
                std::string value; is >> value;
                ud.add("addAttribute", name, value);
            }
            break;
        }
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeAttributes(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addAttribute", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.size() < 2 ) continue;

        std::string name = irArgString(c.args[0]);
        const IRArg& v = c.args[1];
        int type = 0;  // STRING
        if ( v.kind == IRArg::INT || v.kind == IRArg::UINT ) type = 1;  // INTEGER
        else if ( v.kind == IRArg::DOUBLE ) type = 2;  // DOUBLE

        os << os.PROPERTY("ShapeAttribute") << name << std::endl;
        os << os.PROPERTY("Type") << type << std::endl;
        if ( type == 1 ) os << (int)irArgInt(v);
        else if ( type == 2 ) os << irArgDouble(v);
        else os << irArgString(v);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ShapeAttributeList,
                         new osgSim::ShapeAttributeList,
                         osgSim::ShapeAttributeList,
                         "osg::Object osgSim::ShapeAttributeList" )
{
    ADD_USER_SERIALIZER( Attributes );
    ADD_USER_WRITER( Attributes );
}
