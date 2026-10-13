#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgTerrain_ProxyLayer,
                         new osgTerrain::ProxyLayer,
                         osgTerrain::ProxyLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::ProxyLayer" )
{
    ADD_OBJECT_SERIALIZER( Implementation, osgTerrain::Layer, NULL );  // _implementation
}
