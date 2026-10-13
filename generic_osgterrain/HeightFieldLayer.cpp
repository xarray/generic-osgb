#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgTerrain_HeightFieldLayer,
                         new osgTerrain::HeightFieldLayer,
                         osgTerrain::HeightFieldLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::HeightFieldLayer" )
{
    ADD_OBJECT_SERIALIZER( HeightField, osg::HeightField, NULL );  // _heightField
}
