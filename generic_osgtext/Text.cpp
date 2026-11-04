#include <GenericReserializer.h>
using namespace osgVerse;

// _backdropHorizontalOffset, _backdropVerticalOffset

static bool readBackdropOffset(InputStream& is, InputUserData& ud)
{
    float horizontal, vertical; is >> horizontal >> vertical;
    ud.add("setBackdropOffset",  horizontal, vertical );
    return true;
}

// _colorGradientTopLeft .. _colorGradientBottomRight

static bool readColorGradientCorners(InputStream& is, InputUserData& ud)
{
    osg::Vec4d lt, lb, rb, rt;
    is >> is.BEGIN_BRACKET;
    is >> is.PROPERTY("TopLeft") >> lt;
    is >> is.PROPERTY("BottomLeft") >> lb;
    is >> is.PROPERTY("BottomRight") >> rb;
    is >> is.PROPERTY("TopRight") >> rt;
    is >> is.END_BRACKET;
    ud.add("setColorGradientCorners",  lt, lb, rb, rt );
    return true;
}

static bool writeBackdropOffset(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setBackdropOffset");
    float horizontal = 0.0f, vertical = 0.0f;
    if (cmd && cmd->args.size() > 1)
    {
        horizontal = (float)irArgDouble(cmd->args[0]);
        vertical = (float)irArgDouble(cmd->args[1]);
    }
    os << horizontal << vertical << std::endl;
    return true;
}

static bool writeColorGradientCorners(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setColorGradientCorners");
    osg::Vec4d lt, lb, rb, rt;
    if (cmd && cmd->args.size() > 3)
    {
        lt = irArgVec4d(cmd->args[0]); lb = irArgVec4d(cmd->args[1]);
        rb = irArgVec4d(cmd->args[2]); rt = irArgVec4d(cmd->args[3]);
    }

    os << os.BEGIN_BRACKET << std::endl;
    os << os.PROPERTY("TopLeft") << lt << std::endl;
    os << os.PROPERTY("BottomLeft") << lb << std::endl;
    os << os.PROPERTY("BottomRight") << rb << std::endl;
    os << os.PROPERTY("TopRight") << rt << std::endl;
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgText_Text,
                         new osgText::Text,
                         osgText::Text,
                         "osg::Object osg::Node osg::Drawable osgText::TextBase osgText::Text" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::Node")
    }

    ADD_VEC4_SERIALIZER( Color, osg::Vec4() );  // _color

    BEGIN_ENUM_SERIALIZER( BackdropType, NONE );
        ADD_ENUM_VALUE2( DROP_SHADOW_BOTTOM_RIGHT, 0 );
        ADD_ENUM_VALUE2( DROP_SHADOW_CENTER_RIGHT, 1 );
        ADD_ENUM_VALUE2( DROP_SHADOW_TOP_RIGHT, 2 );
        ADD_ENUM_VALUE2( DROP_SHADOW_BOTTOM_CENTER, 3 );
        ADD_ENUM_VALUE2( DROP_SHADOW_TOP_CENTER, 4 );
        ADD_ENUM_VALUE2( DROP_SHADOW_BOTTOM_LEFT, 5 );
        ADD_ENUM_VALUE2( DROP_SHADOW_CENTER_LEFT, 6 );
        ADD_ENUM_VALUE2( DROP_SHADOW_TOP_LEFT, 7 );
        ADD_ENUM_VALUE2( OUTLINE, 8 );
        ADD_ENUM_VALUE2( NONE, 9 );
    END_ENUM_SERIALIZER();  // _backdropType

    BEGIN_ENUM_SERIALIZER( BackdropImplementation, DEPTH_RANGE );
        ADD_ENUM_VALUE2( POLYGON_OFFSET, 0 );
        ADD_ENUM_VALUE2( NO_DEPTH_BUFFER, 1 );
        ADD_ENUM_VALUE2( DEPTH_RANGE, 2 );
        ADD_ENUM_VALUE2( STENCIL_BUFFER, 3 );
    END_ENUM_SERIALIZER();  // _backdropImplementation

    ADD_USER_SERIALIZER( BackdropOffset );  // _backdropHorizontalOffset, _backdropVerticalOffset
    ADD_USER_WRITER( BackdropOffset );
    ADD_VEC4_SERIALIZER( BackdropColor, osg::Vec4() );  // _backdropColor

    BEGIN_ENUM_SERIALIZER( ColorGradientMode, SOLID );
        ADD_ENUM_VALUE2( SOLID, 0 );
        ADD_ENUM_VALUE2( PER_CHARACTER, 1 );
        ADD_ENUM_VALUE2( OVERALL, 2 );
    END_ENUM_SERIALIZER();  // _colorGradientMode

    ADD_USER_SERIALIZER( ColorGradientCorners );  // _colorGradientTopLeft .. _colorGradientBottomRight
    ADD_USER_WRITER( ColorGradientCorners );
}
