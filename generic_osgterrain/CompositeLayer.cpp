#include <GenericReserializer.h>
using namespace osgVerse;

// _layers: a list of either embedded Layer objects ("Object") or file names ("File")
static bool readLayers(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i = 0; i < size; ++i )
    {
        std::string type;
        is >> type;
        if ( type == "Object" )
        {
            ObjectTypeAndID child = ud.readObjectFromStream( is, "osgTerrain::Layer" );
            if ( child.valid() ) ud.add( "addLayer", &child );
        }
        else if ( type == "File" )
        {
            std::string compoundname;
            is.readWrappedString( compoundname );
            ud.add( "addLayer", compoundname );
        }
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_CompositeLayer,
                         new osgTerrain::CompositeLayer,
                         osgTerrain::CompositeLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::CompositeLayer" )
{
    ADD_USER_SERIALIZER( Layers );  // _layers
}
