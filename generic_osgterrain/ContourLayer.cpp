#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgTerrain_ContourLayer,
                         new osgTerrain::ContourLayer,
                         osgTerrain::ContourLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::ContourLayer" )
{
    ADD_OBJECT_SERIALIZER( TransferFunction, osg::TransferFunction1D, NULL );  // _tf
}
