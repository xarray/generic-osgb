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

REGISTER_OBJECT_WRAPPER( osgGA_KeySwitchMatrixManipulator,
                         new osgGA::KeySwitchMatrixManipulator,
                         osgGA::KeySwitchMatrixManipulator,
                         "osg::Object osgGA::KeySwitchMatrixManipulator" )
{
    ADD_USER_SERIALIZER( KeyManipMap );  // _children
}
