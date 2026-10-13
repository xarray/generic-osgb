#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgVolume_FixedFunctionTechnique,
                         new osgVolume::FixedFunctionTechnique,
                         osgVolume::FixedFunctionTechnique,
                         "osg::Object osgVolume::VolumeTechnique osgVolume::FixedFunctionTechnique" )
{
    ADD_UINT_SERIALIZER( NumSlices, 500 );  // _numSlices
}
