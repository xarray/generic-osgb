#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgVolume_TransferFunctionProperty,
                         new osgVolume::TransferFunctionProperty,
                         osgVolume::TransferFunctionProperty,
                         "osg::Object osgVolume::Property osgVolume::TransferFunctionProperty" )
{
    ADD_OBJECT_SERIALIZER( TransferFunction, osg::TransferFunction, NULL );  // _tf
}
