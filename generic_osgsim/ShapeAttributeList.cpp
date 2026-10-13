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

REGISTER_OBJECT_WRAPPER( osgSim_ShapeAttributeList,
                         new osgSim::ShapeAttributeList,
                         osgSim::ShapeAttributeList,
                         "osg::Object osgSim::ShapeAttributeList" )
{
    ADD_USER_SERIALIZER( Attributes );
}
