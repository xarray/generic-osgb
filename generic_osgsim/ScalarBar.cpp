#include <GenericReserializer.h>
using namespace osgVerse;

// _stc

static bool readScalarsToColors(InputStream& is, InputUserData& ud)
{
    float min = 0.0f, max = 0.0f;
    is >> is.BEGIN_BRACKET;
    is >> is.PROPERTY("Range") >> min >> max;

    bool hasColorRange = false;
    unsigned int colorSize = 0;
    is >> is.PROPERTY("Colors") >> hasColorRange >> colorSize;
    ud.add("setScalarsToColors", min, max, hasColorRange, colorSize);
    if ( hasColorRange )
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<colorSize; ++i )
        {
            osg::Vec4 color; is >> color;
            ud.add("setScalarsToColor", i, color);
        }
        is >> is.END_BRACKET;
    }
    return true;
}

// _sp

static bool readScalarPrinter(InputStream& is, InputUserData& ud)
{
    is >> is.BEGIN_BRACKET;
    ObjectTypeAndID sp = ud.readObjectFromStream(is, "osgSim::ScalarBar::ScalarPrinter");
    if ( sp ) ud.add("setScalarPrinter",  sp.get() );
    is >> is.END_BRACKET;
    return true;
}

// _textProperties

static bool readTextProperties(InputStream& is, InputUserData& ud)
{
    std::string fontFile; float characterSize = 0.0f;
    int resX = 0, resY = 0; osg::Vec4 color;
    is >> is.BEGIN_BRACKET;
    is >> is.PROPERTY("Font") >> fontFile;
    is >> is.PROPERTY("Resolution") >> resX >> resY;
    is >> is.PROPERTY("CharacterSize") >> characterSize;
    is >> is.PROPERTY("Color") >> color;
    is >> is.END_BRACKET;
    ud.add("setTextProperties", fontFile, resX, resY, characterSize, color);
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_ScalarBar,
                         new osgSim::ScalarBar,
                         osgSim::ScalarBar,
                         "osg::Object osg::Node osg::Geode osgSim::ScalarBar" )
{
    ADD_INT_SERIALIZER( NumColors, 256 );  // _numColors
    ADD_INT_SERIALIZER( NumLabels, 0 );  // _numLabels
    ADD_USER_SERIALIZER( ScalarsToColors );  // _stc
    ADD_STRING_SERIALIZER( Title, "" );  // _title
    ADD_VEC3_SERIALIZER( Position, osg::Vec3() );  // _position
    ADD_FLOAT_SERIALIZER( Width, 0.0f );  // _width
    ADD_FLOAT_SERIALIZER( AspectRatio, 0.0f );  // _aspectRatio

    BEGIN_ENUM_SERIALIZER( Orientation, HORIZONTAL );
        ADD_ENUM_VALUE( HORIZONTAL );
        ADD_ENUM_VALUE( VERTICAL );
    END_ENUM_SERIALIZER();  // _orientation

    ADD_USER_SERIALIZER( ScalarPrinter );  // _sp
    ADD_USER_SERIALIZER( TextProperties );  // _textProperties
}
