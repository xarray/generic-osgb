#include <GenericReserializer.h>
using namespace osgVerse;

// _fileNames
static bool readFileNames(InputStream& is, InputUserData& ud)
{
    unsigned int files = 0; is >> files >> is.BEGIN_BRACKET;
    //if (is.getOptions()) image.setReadOptions(new osgDB::Options(*is.getOptions()));
    for ( unsigned int i=0; i<files; ++i )
    {
        std::string filename; is.readWrappedString( filename );
        ud.add("addImageFile", filename);
    }
    is >> is.END_BRACKET;
    return true;
}

// _images
static bool readImages(InputStream& is, InputUserData& ud)
{
    unsigned int images = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<images; ++i )
    {
        //osg::ref_ptr<osg::Image> img = is.readImage();
        ObjectTypeAndID img = is.readObject(ud);
        if (img.valid()) ud.add("addImage", &img);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeFileNames(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addImageFile", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        os.writeWrappedString(cmds[i]->args.empty() ? std::string()
                                                    : irArgString(cmds[i]->args[0]));
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeImages(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addImage", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<cmds.size(); ++i )
    {
        const IRCommand& c = *cmds[i];
        os.writeObject(ud, c.args.empty() ? -1 : ud.resolve(c.args[0]));
        if (!os.isBinary()) os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( ImageSequence,
                         new osg::ImageSequence,
                         osg::ImageSequence,
                         "osg::Object osg::BufferData osg::Image osg::ImageStream osg::ImageSequence" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::BufferData")
    }
    ADD_DOUBLE_SERIALIZER( ReferenceTime, DBL_MAX );  // _referenceTime
    ADD_DOUBLE_SERIALIZER( TimeMultiplier, 1.0 );  // _timeMultiplier

    BEGIN_ENUM_SERIALIZER( Mode, PRE_LOAD_ALL_IMAGES );
        ADD_ENUM_VALUE2( PRE_LOAD_ALL_IMAGES, 0 );
        ADD_ENUM_VALUE2( PAGE_AND_RETAIN_IMAGES, 1 );
        ADD_ENUM_VALUE2( PAGE_AND_DISCARD_USED_IMAGES, 2 );
        ADD_ENUM_VALUE2( LOAD_AND_DISCARD_IN_UPDATE_TRAVERSAL, 3 );
        ADD_ENUM_VALUE2( LOAD_AND_RETAIN_IN_UPDATE_TRAVERSAL, 4 );
    END_ENUM_SERIALIZER();  // _mode

    ADD_DOUBLE_SERIALIZER( Length, 1.0 );  // _length
    ADD_USER_SERIALIZER( FileNames );  // _fileNames
    ADD_USER_WRITER( FileNames );
    ADD_USER_SERIALIZER( Images );  // _images
    ADD_USER_WRITER( Images );
}
