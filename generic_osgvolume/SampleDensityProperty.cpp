#include <GenericReserializer.h>
using namespace osgVerse;

namespace osgVolume_SampleDensityProperty
{
    REGISTER_OBJECT_WRAPPER( osgVolume_SampleDensityProperty,
                            new osgVolume::SampleDensityProperty,
                            osgVolume::SampleDensityProperty,
                            "osg::Object osgVolume::Property osgVolume::ScalarProperty osgVolume::SampleDensityProperty" )
    {
    }
}

namespace osgVolume_SampleDensityWhenMovingProperty
{
    REGISTER_OBJECT_WRAPPER( osgVolume_SampleDensityWhenMovingProperty,
                            new osgVolume::SampleDensityWhenMovingProperty,
                            osgVolume::SampleDensityWhenMovingProperty,
                            "osg::Object osgVolume::Property osgVolume::ScalarProperty osgVolume::SampleDensityWhenMovingProperty" )
    {
    }
}
