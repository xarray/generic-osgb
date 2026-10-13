#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgVolume_ScalarProperty,
                         new osgVolume::ScalarProperty("unknown", 0.0f),
                         osgVolume::ScalarProperty,
                         "osg::Object osgVolume::Property osgVolume::ScalarProperty" )
{
    // FIXME: don't set constructor directly, try implementing a setName() method?

    ADD_FLOAT_SERIALIZER( Value, 1.0f );
}
