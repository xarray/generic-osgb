#include <GenericReserializer.h>
using namespace osgVerse;

// _font

static bool readFont(InputStream& is, InputUserData& ud)
{
    std::string fontName; is.readWrappedString( fontName );
    ud.add("setFont", fontName);
    return true;
}

// _fontSize

static bool readFontSize(InputStream& is, InputUserData& ud)
{
    unsigned int width, height; is >> width >> height;
    ud.add("setFontResolution",  width, height );
    return true;
}

// _characterHeight, _characterAspectRatio

static bool readCharacterSize(InputStream& is, InputUserData& ud)
{
    float height, aspectRatio; is >> height >> aspectRatio;
    ud.add("setCharacterSize",  height, aspectRatio );
    return true;
}

// _text

static bool readText(InputStream& is, InputUserData& ud)
{
    bool isACString = true; is >> isACString;
    if ( isACString )
    {
        std::string acString; is.readWrappedString( acString );
        ud.add("setText", acString);
    }
    else
    {
        ObjectTypeAndID array = is.readArray(ud);
        if ( array.valid() ) ud.add("setTextArray", &array);
    }
    return true;
}

static bool writeFont(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setFont");
    os.writeWrappedString((cmd && !cmd->args.empty()) ? irArgString(cmd->args[0]) : std::string());
    os << std::endl;
    return true;
}

static bool writeFontSize(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setFontResolution");
    unsigned int width = 0, height = 0;
    if (cmd && cmd->args.size() > 1)
    {
        width = (unsigned int)irArgInt(cmd->args[0]);
        height = (unsigned int)irArgInt(cmd->args[1]);
    }
    os << width << height << std::endl;
    return true;
}

static bool writeCharacterSize(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setCharacterSize");
    float height = 0.0f, aspectRatio = 0.0f;
    if (cmd && cmd->args.size() > 1)
    {
        height = (float)irArgDouble(cmd->args[0]);
        aspectRatio = (float)irArgDouble(cmd->args[1]);
    }
    os << height << aspectRatio << std::endl;
    return true;
}

static bool writeText(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* str = ud.findCommand("setText");
    const IRCommand* arr = ud.findCommand("setTextArray");
    if (str || !arr || arr->args.empty())
    {
        os << true;
        os.writeWrappedString(str && !str->args.empty() ? irArgString(str->args[0])
                                                       : std::string());
        os << std::endl;
    }
    else
    {
        os << false;
        os.writeCompactArray(ud, ud.resolve(arr->args[0]));
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( osgText_TextBase,
                         /*new osgText::TextBase*/NULL,
                         osgText::TextBase,
                         "osg::Object osg::Node osg::Drawable osgText::TextBase" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::Node")
    }

    ADD_USER_SERIALIZER( Font );  // _font
    ADD_USER_WRITER( Font );
    ADD_USER_SERIALIZER( FontSize );  // _fontSize
    ADD_USER_WRITER( FontSize );
    ADD_USER_SERIALIZER( CharacterSize );  // _characterHeight, _characterAspectRatio
    ADD_USER_WRITER( CharacterSize );

    BEGIN_ENUM_SERIALIZER( CharacterSizeMode, OBJECT_COORDS );
        ADD_ENUM_VALUE2( OBJECT_COORDS, 0 );
        ADD_ENUM_VALUE2( SCREEN_COORDS, 1 );
        ADD_ENUM_VALUE2( OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT, 2 );
    END_ENUM_SERIALIZER();  // _characterSizeMode

    ADD_FLOAT_SERIALIZER( MaximumWidth, 0.0f );  // _maximumWidth
    ADD_FLOAT_SERIALIZER( MaximumHeight, 0.0f );  // _maximumHeight
    ADD_FLOAT_SERIALIZER( LineSpacing, 0.0f );  // _lineSpacing
    ADD_USER_SERIALIZER( Text );  // _text
    ADD_USER_WRITER( Text );
    ADD_VEC3_SERIALIZER( Position, osg::Vec3() );  // _position

    BEGIN_ENUM_SERIALIZER2( Alignment, osgText::TextBase::AlignmentType, LEFT_BASE_LINE );
        ADD_ENUM_VALUE2( LEFT_TOP, 0 );
        ADD_ENUM_VALUE2( LEFT_CENTER, 1 );
        ADD_ENUM_VALUE2( LEFT_BOTTOM, 2 );
        ADD_ENUM_VALUE2( CENTER_TOP, 3 );
        ADD_ENUM_VALUE2( CENTER_CENTER, 4 );
        ADD_ENUM_VALUE2( CENTER_BOTTOM, 5 );
        ADD_ENUM_VALUE2( RIGHT_TOP, 6 );
        ADD_ENUM_VALUE2( RIGHT_CENTER, 7 );
        ADD_ENUM_VALUE2( RIGHT_BOTTOM, 8 );
        ADD_ENUM_VALUE2( LEFT_BASE_LINE, 9 );
        ADD_ENUM_VALUE2( CENTER_BASE_LINE, 10 );
        ADD_ENUM_VALUE2( RIGHT_BASE_LINE, 11 );
        ADD_ENUM_VALUE2( LEFT_BOTTOM_BASE_LINE, 12 );
        ADD_ENUM_VALUE2( CENTER_BOTTOM_BASE_LINE, 13 );
        ADD_ENUM_VALUE2( RIGHT_BOTTOM_BASE_LINE, 14 );
    END_ENUM_SERIALIZER();  // _alignment

    BEGIN_ENUM_SERIALIZER( AxisAlignment, XY_PLANE );
        ADD_ENUM_VALUE2( XY_PLANE, 0 );
        ADD_ENUM_VALUE2( REVERSED_XY_PLANE, 1 );
        ADD_ENUM_VALUE2( XZ_PLANE, 2 );
        ADD_ENUM_VALUE2( REVERSED_XZ_PLANE, 3 );
        ADD_ENUM_VALUE2( YZ_PLANE, 4 );
        ADD_ENUM_VALUE2( REVERSED_YZ_PLANE, 5 );
        ADD_ENUM_VALUE2( SCREEN, 6 );
        ADD_ENUM_VALUE2( USER_DEFINED_ROTATION, 7 );
    END_ENUM_SERIALIZER();  // _axisAlignment

    ADD_QUAT_SERIALIZER( Rotation, osg::Quat() );  // _rotation
    ADD_BOOL_SERIALIZER( AutoRotateToScreen, false );  // _autoRotateToScreen

    BEGIN_ENUM_SERIALIZER( Layout, LEFT_TO_RIGHT );
        ADD_ENUM_VALUE2( LEFT_TO_RIGHT, 0 );
        ADD_ENUM_VALUE2( RIGHT_TO_LEFT, 1 );
        ADD_ENUM_VALUE2( VERTICAL, 2 );
    END_ENUM_SERIALIZER();  // _layout

    BEGIN_BITFLAGS_SERIALIZER(DrawMode, 1);  // osgText::TextBase::TEXT
        ADD_BITFLAG_VALUE(TEXT, 1);
        ADD_BITFLAG_VALUE(BOUND, 2);       // BOUNDINGBOX
        ADD_BITFLAG_VALUE(FILLED, 4);      // FILLEDBOUNDINGBOX
        ADD_BITFLAG_VALUE(ALIGNMENT, 8);
    END_BITFLAGS_SERIALIZER();
    ADD_FLOAT_SERIALIZER( BoundingBoxMargin, 0.0f );  // _textBBMargin
    ADD_VEC4_SERIALIZER( BoundingBoxColor, osg::Vec4() );  // _textBBColor
}
