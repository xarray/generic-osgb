#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgVolume_Volume,
                         new osgVolume::Volume,
                         osgVolume::Volume,
                         "osg::Object osg::Node osg::Group osgVolume::Volume" )
{
    ADD_OBJECT_SERIALIZER( VolumeTechniquePrototype, osgVolume::VolumeTechnique, NULL );  // _volumeTechnique
}
