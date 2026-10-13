#include <GenericReserializer.h>
using namespace osgVerse;

static bool readChildren(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        // 数组元素是直接对象（className 开头），无 hasObject 标志
        ObjectTypeAndID obj = is.readObject(ud);
        if (obj.valid()) ud.add("addChild", &obj);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( Group,
                         new osg::Group,
                         osg::Group,
                         "osg::Object osg::Node osg::Group" )
{
    ADD_USER_SERIALIZER( Children );  // _children
}
