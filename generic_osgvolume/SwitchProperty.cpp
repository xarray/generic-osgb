#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgVolume_SwitchProperty,
                         new osgVolume::SwitchProperty,
                         osgVolume::SwitchProperty,
                         "osg::Object osgVolume::Property osgVolume::CompositeProperty osgVolume::SwitchProperty" )
{
    ADD_INT_SERIALIZER( ActiveProperty, 0 );  // _activeProperty
}
