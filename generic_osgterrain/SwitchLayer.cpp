#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgTerrain_SwitchLayer,
                         new osgTerrain::SwitchLayer,
                         osgTerrain::SwitchLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::CompositeLayer osgTerrain::SwitchLayer" )
{
    ADD_INT_SERIALIZER( ActiveLayer, -1 );  // _activeLayer
}
