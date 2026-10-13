#include <GenericReserializer.h>
using namespace osgVerse;

#define OBJECT_CAST dynamic_cast

REGISTER_OBJECT_WRAPPER( osgGA_TerrainManipulator,
                         new osgGA::TerrainManipulator,
                         osgGA::TerrainManipulator,
                         "osg::Object osgGA::TerrainManipulator" )
{
}
