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

static bool writeLayers(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addLayer", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        if ( c.args.empty() ) continue;
        const IRArg& a = c.args[0];

        if ( a.kind == IRArg::OBJECT )
        {
            int target = ud.resolve(a);
            os << std::string("Object");
            if ( os.isBinary() ) os << (target >= 0);
            else os << std::endl;
            if ( target >= 0 ) os.writeObject(ud, target);
            if ( !os.isBinary() ) os << std::endl;
        }
        else
        {
            os << std::string("File");
            os.writeWrappedString(irArgString(a));
            os << std::endl;
        }
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_CompositeLayer,
                         new osgTerrain::CompositeLayer,
                         osgTerrain::CompositeLayer,
                         "osg::Object osgTerrain::Layer osgTerrain::CompositeLayer" )
{
    ADD_USER_SERIALIZER( Layers );  // _layers
    ADD_USER_WRITER( Layers );
}
