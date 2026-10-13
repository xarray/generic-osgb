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
        ADD_ENUM_VALUE( DROP_SHADOW_BOTTOM_RIGHT );
        ADD_ENUM_VALUE( DROP_SHADOW_CENTER_RIGHT );
        ADD_ENUM_VALUE( DROP_SHADOW_TOP_RIGHT );
        ADD_ENUM_VALUE( DROP_SHADOW_BOTTOM_CENTER );
        ADD_ENUM_VALUE( DROP_SHADOW_TOP_CENTER );
        ADD_ENUM_VALUE( DROP_SHADOW_BOTTOM_LEFT );
        ADD_ENUM_VALUE( DROP_SHADOW_CENTER_LEFT );
        ADD_ENUM_VALUE( DROP_SHADOW_TOP_LEFT );
        ADD_ENUM_VALUE( OUTLINE );
        ADD_ENUM_VALUE( NONE );
    END_ENUM_SERIALIZER();  // _backdropType

    BEGIN_ENUM_SERIALIZER( BackdropImplementation, DEPTH_RANGE );
        ADD_ENUM_VALUE( POLYGON_OFFSET );
        ADD_ENUM_VALUE( NO_DEPTH_BUFFER );
        ADD_ENUM_VALUE( DEPTH_RANGE );
        ADD_ENUM_VALUE( STENCIL_BUFFER );
    END_ENUM_SERIALIZER();  // _backdropImplementation

    ADD_USER_SERIALIZER( BackdropOffset );  // _backdropHorizontalOffset, _backdropVerticalOffset
    ADD_VEC4_SERIALIZER( BackdropColor, osg::Vec4() );  // _backdropColor

    BEGIN_ENUM_SERIALIZER( ColorGradientMode, SOLID );
        ADD_ENUM_VALUE( SOLID );
        ADD_ENUM_VALUE( PER_CHARACTER );
        ADD_ENUM_VALUE( OVERALL );
    END_ENUM_SERIALIZER();  // _colorGradientMode

    ADD_USER_SERIALIZER( ColorGradientCorners );  // _colorGradientTopLeft .. _colorGradientBottomRight
}
