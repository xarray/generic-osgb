#include <GenericReserializer.h>
using namespace osgVerse;

static void readConvexPlanarPolygon(InputStream& is, InputUserData& ud,
                                    const char* command, int holeIndex = -1)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        osg::Vec3d vertex; is >> vertex;
        if (holeIndex < 0) ud.add(command, vertex);
        else ud.add(command, (unsigned int)holeIndex, vertex);
    }
    is >> is.END_BRACKET;
}

static bool readOccluder(InputStream& is, InputUserData& ud)
{
    readConvexPlanarPolygon( is, ud, "polygon");
    return true;
}

static bool readHoles(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("Polygon");
        readConvexPlanarPolygon(is, ud, "holePolygon", (int)i);
    }
    is >> is.END_BRACKET;
    return true;
}

static void writePolygon(OutputStream& os, const std::vector<const IRCommand*>& cmds,
                         unsigned int argIndex = 0)
{
    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        os << (c.args.size() > argIndex ? irArgVec3d(c.args[argIndex]) : osg::Vec3d())
           << std::endl;
    }
    os << os.END_BRACKET << std::endl;
}

static bool writeOccluder(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("polygon", cmds);
    writePolygon(os, cmds);
    return true;
}

static bool writeHoles(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("holePolygon", cmds);

    unsigned int count = 0;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        if (!cmds[i]->args.empty())
            count = std::max(count, (unsigned int)irArgInt(cmds[i]->args[0]) + 1);
    }

    os << count << os.BEGIN_BRACKET << std::endl;
    for ( unsigned int h=0; h<count; ++h )
    {
        std::vector<const IRCommand*> polygon;
        for ( size_t i=0; i<cmds.size(); ++i )
        {
            if (!cmds[i]->args.empty() && (unsigned int)irArgInt(cmds[i]->args[0]) == h)
                polygon.push_back(cmds[i]);
        }
        os << os.PROPERTY("Polygon");
        writePolygon(os, polygon, 1);
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( ConvexPlanarOccluder,
                         new osg::ConvexPlanarOccluder,
                         osg::ConvexPlanarOccluder,
                         "osg::Object osg::ConvexPlanarOccluder" )
{
    ADD_USER_SERIALIZER( Occluder );  // _occluder
    ADD_USER_WRITER( Occluder );
    ADD_USER_SERIALIZER( Holes );  // _holeList
    ADD_USER_WRITER( Holes );
}
