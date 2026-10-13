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
    ADD_USER_SERIALIZER( FontSize );  // _fontSize
    ADD_USER_SERIALIZER( CharacterSize );  // _characterHeight, _characterAspectRatio

    BEGIN_ENUM_SERIALIZER( CharacterSizeMode, OBJECT_COORDS );
        ADD_ENUM_VALUE( OBJECT_COORDS );
        ADD_ENUM_VALUE( SCREEN_COORDS );
        ADD_ENUM_VALUE( OBJECT_COORDS_WITH_MAXIMUM_SCREEN_SIZE_CAPPED_BY_FONT_HEIGHT );
    END_ENUM_SERIALIZER();  // _characterSizeMode

    ADD_FLOAT_SERIALIZER( MaximumWidth, 0.0f );  // _maximumWidth
    ADD_FLOAT_SERIALIZER( MaximumHeight, 0.0f );  // _maximumHeight
    ADD_FLOAT_SERIALIZER( LineSpacing, 0.0f );  // _lineSpacing
    ADD_USER_SERIALIZER( Text );  // _text
    ADD_VEC3_SERIALIZER( Position, osg::Vec3() );  // _position

    BEGIN_ENUM_SERIALIZER2( Alignment, osgText::TextBase::AlignmentType, LEFT_BASE_LINE );
        ADD_ENUM_VALUE( LEFT_TOP );
        ADD_ENUM_VALUE( LEFT_CENTER );
        ADD_ENUM_VALUE( LEFT_BOTTOM );
        ADD_ENUM_VALUE( CENTER_TOP );
        ADD_ENUM_VALUE( CENTER_CENTER );
        ADD_ENUM_VALUE( CENTER_BOTTOM );
        ADD_ENUM_VALUE( RIGHT_TOP );
        ADD_ENUM_VALUE( RIGHT_CENTER );
        ADD_ENUM_VALUE( RIGHT_BOTTOM );
        ADD_ENUM_VALUE( LEFT_BASE_LINE );
        ADD_ENUM_VALUE( CENTER_BASE_LINE );
        ADD_ENUM_VALUE( RIGHT_BASE_LINE );
        ADD_ENUM_VALUE( LEFT_BOTTOM_BASE_LINE );
        ADD_ENUM_VALUE( CENTER_BOTTOM_BASE_LINE );
        ADD_ENUM_VALUE( RIGHT_BOTTOM_BASE_LINE );
    END_ENUM_SERIALIZER();  // _alignment

    BEGIN_ENUM_SERIALIZER( AxisAlignment, XY_PLANE );
        ADD_ENUM_VALUE( XY_PLANE );
        ADD_ENUM_VALUE( REVERSED_XY_PLANE );
        ADD_ENUM_VALUE( XZ_PLANE );
        ADD_ENUM_VALUE( REVERSED_XZ_PLANE );
        ADD_ENUM_VALUE( YZ_PLANE );
        ADD_ENUM_VALUE( REVERSED_YZ_PLANE );
        ADD_ENUM_VALUE( SCREEN );
        ADD_ENUM_VALUE( USER_DEFINED_ROTATION );
    END_ENUM_SERIALIZER();  // _axisAlignment

    ADD_QUAT_SERIALIZER( Rotation, osg::Quat() );  // _rotation
    ADD_BOOL_SERIALIZER( AutoRotateToScreen, false );  // _autoRotateToScreen

    BEGIN_ENUM_SERIALIZER( Layout, LEFT_TO_RIGHT );
        ADD_ENUM_VALUE( LEFT_TO_RIGHT );
        ADD_ENUM_VALUE( RIGHT_TO_LEFT );
        ADD_ENUM_VALUE( VERTICAL );
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
