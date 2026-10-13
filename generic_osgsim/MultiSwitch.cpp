#include <GenericReserializer.h>
using namespace osgVerse;

static bool readValues(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("SwitchSet");
        unsigned int valueSize = is.readSize(); is >> is.BEGIN_BRACKET;
        ud.add("setValueList", i, valueSize);

        for ( unsigned int j=0; j<valueSize; ++j )
        {
            bool value = false; is >> value;
            ud.add("setValue", i, j, value);
        }
        is >> is.END_BRACKET;
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_MultiSwitch,
                         new osgSim::MultiSwitch,
                         osgSim::MultiSwitch,
                         "osg::Object osg::Node osg::Group osgSim::MultiSwitch" )
{
    ADD_BOOL_SERIALIZER( NewChildDefaultValue, true );  // _newChildDefaultValue
    ADD_UINT_SERIALIZER( ActiveSwitchSet, 0 );  // _activeSwitchSet
    ADD_USER_SERIALIZER( Values );  // _values
}
