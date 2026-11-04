#include <GenericReserializer.h>
using namespace osgVerse;

enum osg_StateAttribute
{
    /** means that associated GLMode and Override is disabled.*/
    OFF = 0x0,
    /** means that associated GLMode is enabled and Override is disabled.*/
    ON = 0x1,
    /** Overriding of GLMode's or StateAttributes is enabled, so that state below it is overridden.*/
    OVERRIDE = 0x2,
    /** Protecting of GLMode's or StateAttributes is enabled, so that state from above cannot override this and below state.*/
    PROTECTED = 0x4,
    /** means that GLMode or StateAttribute should be inherited from above.*/
    INHERIT = 0x8
};

static int readValue(InputStream& is)
{
    int value = 0;
    if ( is.isBinary() )
        is >> value;
    else
    {
        std::string enumValue;
        is >> enumValue;

        if (enumValue.find("OFF")!=std::string::npos) value = osg_StateAttribute::OFF;
        if (enumValue.find("ON")!=std::string::npos) value = osg_StateAttribute::ON;
        if (enumValue.find("OVERRIDE")!=std::string::npos) value = value | osg_StateAttribute::OVERRIDE;
        if (enumValue.find("PROTECTED")!=std::string::npos) value = value | osg_StateAttribute::PROTECTED;
        if (enumValue.find("INHERIT")!=std::string::npos) value = value | osg_StateAttribute::INHERIT;
    }
    return value;
}

static void readModes(InputStream& is, std::map<GLenum, unsigned int>& modes)
{
    unsigned int size = is.readSize();
    if ( size>0 )
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            DEF_GLENUM(mode); is >> mode;
            int value = readValue( is );
            modes[mode.get()] = value;
        }
        is >> is.END_BRACKET;
    }
}

typedef std::pair<ObjectTypeAndID, int> AttributePair;
static void readAttributes(InputStream& is, InputUserData& ud, std::vector<AttributePair>& attrs)
{
    unsigned int size = is.readSize();
    if ( size>0 )
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            ObjectTypeAndID sa = is.readObject(ud);
            is >> is.PROPERTY("Value");
            int value = readValue( is );
            if ( sa.valid() )
                attrs.push_back(AttributePair(sa, value));
        }
        is >> is.END_BRACKET;
    }
}

// _modeList
static bool readModeList(InputStream& is, InputUserData& ud)
{
    std::map<GLenum, unsigned int> modes; readModes( is, modes );
    for ( std::map<GLenum, unsigned int>::iterator itr=modes.begin();
          itr!=modes.end(); ++itr )
    {
        ud.add("setMode", itr->first, itr->second);
    }
    return true;
}

// _attributeList
static bool readAttributeList(InputStream& is, InputUserData& ud)
{
    std::vector<AttributePair> attrs; readAttributes( is, ud, attrs );
    for ( std::vector<AttributePair>::iterator itr=attrs.begin();
          itr!=attrs.end(); ++itr )
    {
        ud.add("setAttribute", &(itr->first), itr->second);
    }
    return true;
}

// _textureModeList
static bool readTextureModeList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    std::map<GLenum, unsigned int> modes;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("Data");
        readModes( is, modes );
        for ( std::map<GLenum, unsigned int>::iterator itr=modes.begin();
              itr!=modes.end(); ++itr )
        {
            ud.add("setTextureMode", i, itr->first, itr->second);
        }
        modes.clear();
    }
    is >> is.END_BRACKET;
    return true;
}

// _textureAttributeList
static bool readTextureAttributeList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    std::vector<AttributePair> attrs;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("Data");
        readAttributes( is, ud, attrs );
        for ( std::vector<AttributePair>::iterator itr=attrs.begin();
              itr!=attrs.end(); ++itr )
        {
            ud.add("setTextureAttribute", i, &(itr->first), itr->second);
        }
        attrs.clear();
    }
    is >> is.END_BRACKET;
    return true;
}

// _uniformList
static bool readUniformList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID uniform = is.readObject(ud);
        is >> is.PROPERTY("Value");
        int value = readValue( is );
        if ( uniform.valid() )
            ud.add("addUniform", &uniform, value);
    }
    is >> is.END_BRACKET;
    return true;
}

// _defineList
static bool readDefineList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string defineName;
        is.readWrappedString( defineName );

        std::string defineValue;
        is.readWrappedString( defineValue );

        is >> is.PROPERTY("Value");
        int overrideValue = readValue( is );

        ud.add("setDefine", defineName, defineValue, overrideValue);
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeValue(OutputStream& os, int value)
{
    if (os.isBinary()) { os << value; return true; }

    std::string s;
    if ((value & osg_StateAttribute::ON) != 0) s += "ON";
    if ((value & osg_StateAttribute::OVERRIDE) != 0) { if (!s.empty()) s += '|'; s += "OVERRIDE"; }
    if ((value & osg_StateAttribute::PROTECTED) != 0) { if (!s.empty()) s += '|'; s += "PROTECTED"; }
    if ((value & osg_StateAttribute::INHERIT) != 0) { if (!s.empty()) s += '|'; s += "INHERIT"; }
    if (s.empty()) s = "OFF";
    os << s;
    return true;
}

typedef std::pair<unsigned int, int> ModeEntry;
typedef std::pair<int, int> AttributeEntry;

static bool writeModes(OutputStream& os, const std::vector<ModeEntry>& modes)
{
    os << (unsigned int)modes.size();
    if (!modes.empty())
    {
        os << os.BEGIN_BRACKET << std::endl;
        for (size_t i = 0; i < modes.size(); ++i)
        {
            os << ObjectGLenum(modes[i].first, "GL");
            writeValue(os, modes[i].second);
            os << std::endl;
        }
        os << os.END_BRACKET;
    }
    os << std::endl;
    return true;
}

static bool writeAttributes(OutputStream& os, OutputUserData& ud,
                            const std::vector<AttributeEntry>& attrs)
{
    os << (unsigned int)attrs.size();
    if (!attrs.empty())
    {
        os << os.BEGIN_BRACKET << std::endl;
        for (size_t i = 0; i < attrs.size(); ++i)
        {
            os.writeObject(ud, attrs[i].first);
            os << os.PROPERTY("Value");
            writeValue(os, attrs[i].second);
            os << std::endl;
        }
        os << os.END_BRACKET;
    }
    os << std::endl;
    return true;
}

static bool writeModeList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setMode", cmds);
    std::vector<ModeEntry> modes;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand& c = *cmds[i];
        if (c.args.size() < 2) continue;
        modes.push_back(ModeEntry((unsigned int)irArgInt(c.args[0]),
                                  (int)irArgInt(c.args[1])));
    }
    return writeModes(os, modes);
}

static bool writeAttributeList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setAttribute", cmds);
    std::vector<AttributeEntry> attrs;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand& c = *cmds[i];
        if (c.args.size() < 2) continue;
        attrs.push_back(AttributeEntry(ud.resolve(c.args[0]), (int)irArgInt(c.args[1])));
    }
    return writeAttributes(os, ud, attrs);
}

static bool writeTextureModeList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setTextureMode", cmds);

    std::map<unsigned int, std::vector<ModeEntry> > groups;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand& c = *cmds[i];
        if (c.args.size() < 3) continue;
        groups[(unsigned int)irArgInt(c.args[0])].push_back(
            ModeEntry((unsigned int)irArgInt(c.args[1]), (int)irArgInt(c.args[2])));
    }

    unsigned int count = groups.empty() ? 0 : (groups.rbegin()->first + 1);
    os << count << os.BEGIN_BRACKET << std::endl;
    for (unsigned int i = 0; i < count; ++i)
    {
        os << os.PROPERTY("Data");
        std::map<unsigned int, std::vector<ModeEntry> >::iterator itr = groups.find(i);
        static const std::vector<ModeEntry> s_empty;
        writeModes(os, itr != groups.end() ? itr->second : s_empty);
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeTextureAttributeList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setTextureAttribute", cmds);

    std::map<unsigned int, std::vector<AttributeEntry> > groups;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand& c = *cmds[i];
        if (c.args.size() < 3) continue;
        groups[(unsigned int)irArgInt(c.args[0])].push_back(
            AttributeEntry(ud.resolve(c.args[1]), (int)irArgInt(c.args[2])));
    }

    unsigned int count = groups.empty() ? 0 : (groups.rbegin()->first + 1);
    os << count << os.BEGIN_BRACKET << std::endl;
    for (unsigned int i = 0; i < count; ++i)
    {
        os << os.PROPERTY("Data");
        std::map<unsigned int, std::vector<AttributeEntry> >::iterator itr = groups.find(i);
        static const std::vector<AttributeEntry> s_empty;
        writeAttributes(os, ud, itr != groups.end() ? itr->second : s_empty);
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeUniformList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("addUniform", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand& c = *cmds[i];
        if (c.args.empty()) continue;
        os.writeObject(ud, ud.resolve(c.args[0]));
        os << os.PROPERTY("Value");
        writeValue(os, c.args.size() > 1 ? (int)irArgInt(c.args[1]) : 0);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeDefineList(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("setDefine", cmds);

    os << (unsigned int)cmds.size() << os.BEGIN_BRACKET << std::endl;
    for (size_t i = 0; i < cmds.size(); ++i)
    {
        const IRCommand& c = *cmds[i];
        os.writeWrappedString(c.args.size() > 0 ? irArgString(c.args[0]) : std::string());
        os.writeWrappedString(c.args.size() > 1 ? irArgString(c.args[1]) : std::string());
        os << os.PROPERTY("Value");
        writeValue(os, c.args.size() > 2 ? (int)irArgInt(c.args[2]) : 0);
        os << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( StateSet,
                         new osg::StateSet,
                         osg::StateSet,
                         "osg::Object osg::StateSet" )
{
    ADD_USER_SERIALIZER( ModeList );  // _modeList
    ADD_USER_WRITER( ModeList );
    ADD_USER_SERIALIZER( AttributeList );  // _attributeList
    ADD_USER_WRITER( AttributeList );
    ADD_USER_SERIALIZER( TextureModeList );  // _textureModeList
    ADD_USER_WRITER( TextureModeList );
    ADD_USER_SERIALIZER( TextureAttributeList );  // _textureAttributeList
    ADD_USER_WRITER( TextureAttributeList );
    ADD_USER_SERIALIZER( UniformList );  // _uniformList
    ADD_USER_WRITER( UniformList );
    ADD_INT_SERIALIZER( RenderingHint, osg::StateSet::DEFAULT_BIN );  // _renderingHint

    BEGIN_ENUM_SERIALIZER( RenderBinMode, INHERIT_RENDERBIN_DETAILS );
        ADD_ENUM_VALUE2( INHERIT_RENDERBIN_DETAILS, 0 );
        ADD_ENUM_VALUE2( USE_RENDERBIN_DETAILS, 1 );
        ADD_ENUM_VALUE2( OVERRIDE_RENDERBIN_DETAILS, 2 );
        ADD_ENUM_VALUE2( PROTECTED_RENDERBIN_DETAILS, 4 );
        ADD_ENUM_VALUE2( OVERRIDE_PROTECTED_RENDERBIN_DETAILS, 6 );
    END_ENUM_SERIALIZER();  // _binMode

    ADD_INT_SERIALIZER( BinNumber, 0 );  // _binNum
    ADD_STRING_SERIALIZER( BinName, "" );  // _binName
    ADD_BOOL_SERIALIZER( NestRenderBins, true );  // _nestRenderBins
    ADD_OBJECT_SERIALIZER( UpdateCallback, osg::StateSet::Callback, NULL );  // _updateCallback
    ADD_OBJECT_SERIALIZER( EventCallback, osg::StateSet::Callback, NULL );  // _eventCallback

    {
        UPDATE_TO_VERSION_SCOPED( 151 )
        ADD_USER_SERIALIZER( DefineList );  // _defineList
        ADD_USER_WRITER( DefineList );
    }
}
