#include <GenericReserializer.h>
using namespace osgVerse;

static bool readLayers(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID child = ud.readObjectFromStream(is, "osgVolume::Layer");
        if ( child ) ud.add("addLayer",  child );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgVolume_CompositeLayer,
                         new osgVolume::CompositeLayer,
                         osgVolume::CompositeLayer,
                         "osg::Object osgVolume::Layer osgVolume::CompositeLayer" )
{
    ADD_USER_SERIALIZER( Layers );  // _layers
}
