#include <GenericReserializer.h>
using namespace osgVerse;

namespace osgVolume_SampleRatioProperty
{
    REGISTER_OBJECT_WRAPPER( osgVolume_SampleRatioProperty,
                            new osgVolume::SampleRatioProperty,
                            osgVolume::SampleRatioProperty,
                            "osg::Object osgVolume::Property osgVolume::ScalarProperty osgVolume::SampleRatioProperty" )
    {
    }
}

namespace osgVolume_SampleRatioWhenMovingProperty
{
    REGISTER_OBJECT_WRAPPER( osgVolume_SampleRatioWhenMovingProperty,
                            new osgVolume::SampleRatioWhenMovingProperty,
                            osgVolume::SampleRatioWhenMovingProperty,
                            "osg::Object osgVolume::Property osgVolume::ScalarProperty osgVolume::SampleRatioWhenMovingProperty" )
    {
    }
}
