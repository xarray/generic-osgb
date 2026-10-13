#include <GenericReserializer.h>
using namespace osgVerse;

static bool readProperties(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID child = ud.readObjectFromStream(is, "osgVolume::Property");
        if ( child ) ud.add("addProperty",  child );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgVolume_CompositeProperty,
                         new osgVolume::CompositeProperty,
                         osgVolume::CompositeProperty,
                         "osg::Object osgVolume::Property osgVolume::CompositeProperty" )
{
    ADD_USER_SERIALIZER( Properties );  // _properties
}
