#include <GenericReserializer.h>
using namespace osgVerse;

static bool readLocatorCallbacks(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID cb = ud.readObjectFromStream(is, "osgVolume::Locator::LocatorCallback");
        if ( cb ) ud.add("addCallback",  cb );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgVolume_Locator,
                         new osgVolume::Locator,
                         osgVolume::Locator,
                         "osg::Object osgVolume::Locator" )
{
    ADD_MATRIXD_SERIALIZER( Transform, osg::Matrixd() );  // _transform

    ADD_USER_SERIALIZER( LocatorCallbacks );
    {
        UPDATE_TO_VERSION_SCOPED( 90 )
        REMOVE_SERIALIZER( LocatorCallbacks );
    }
}
