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

static bool writeScalarsToColors(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setScalarsToColors");
    float min = 0.0f, max = 1.0f;
    bool hasColorRange = false;
    unsigned int colorSize = 0;
    if (cmd && cmd->args.size() > 3)
    {
        min = (float)irArgDouble(cmd->args[0]);
        max = (float)irArgDouble(cmd->args[1]);
        hasColorRange = irArgBool(cmd->args[2]);
        colorSize = (unsigned int)irArgInt(cmd->args[3]);
    }

    os << os.BEGIN_BRACKET << std::endl;
    os << os.PROPERTY("Range") << min << max << std::endl;
    os << os.PROPERTY("Colors");
    if ( hasColorRange )
    {
        std::vector<const IRCommand*> colors;
        ud.findCommands("setScalarsToColor", colors);

        os << true << colorSize << os.BEGIN_BRACKET << std::endl;
        for ( unsigned int i=0; i<colorSize; ++i )
        {
            osg::Vec4f color;
            for ( size_t k=0; k<colors.size(); ++k )
            {
                const IRCommand& c = *colors[k];
                if (c.args.size() > 1 && (unsigned int)irArgInt(c.args[0]) == i)
                { color = irArgVec4f(c.args[1]); break; }
            }
            os << color << std::endl;
        }
        os << os.END_BRACKET << std::endl;
    }
    else
        os << false << colorSize << std::endl;
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeScalarPrinter(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setScalarPrinter");
    int target = (cmd && !cmd->args.empty()) ? ud.resolve(cmd->args[0]) : -1;

    os << os.BEGIN_BRACKET << std::endl;
    if ( os.isBinary() ) os << (target >= 0);
    if ( target >= 0 ) os.writeObject(ud, target);
    os << os.END_BRACKET << std::endl;
    return true;
}

static bool writeTextProperties(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setTextProperties");
    std::string fontFile;
    int resX = 0, resY = 0;
    float characterSize = 0.0f;
    osg::Vec4f color;
    if (cmd && cmd->args.size() > 4)
    {
        fontFile = irArgString(cmd->args[0]);
        resX = (int)irArgInt(cmd->args[1]);
        resY = (int)irArgInt(cmd->args[2]);
        characterSize = (float)irArgDouble(cmd->args[3]);
        color = irArgVec4f(cmd->args[4]);
    }

    os << os.BEGIN_BRACKET << std::endl;
    os << os.PROPERTY("Font") << fontFile << std::endl;
    os << os.PROPERTY("Resolution") << resX << resY << std::endl;
    os << os.PROPERTY("CharacterSize") << characterSize << std::endl;
    os << os.PROPERTY("Color") << color << std::endl;
    os << os.END_BRACKET << std::endl;
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
    ADD_USER_WRITER( ScalarsToColors );
    ADD_STRING_SERIALIZER( Title, "" );  // _title
    ADD_VEC3_SERIALIZER( Position, osg::Vec3() );  // _position
    ADD_FLOAT_SERIALIZER( Width, 0.0f );  // _width
    ADD_FLOAT_SERIALIZER( AspectRatio, 0.0f );  // _aspectRatio

    BEGIN_ENUM_SERIALIZER( Orientation, HORIZONTAL );
        ADD_ENUM_VALUE2( HORIZONTAL, 0 );
        ADD_ENUM_VALUE2( VERTICAL, 1 );
    END_ENUM_SERIALIZER();  // _orientation

    ADD_USER_SERIALIZER( ScalarPrinter );  // _sp
    ADD_USER_WRITER( ScalarPrinter );
    ADD_USER_SERIALIZER( TextProperties );  // _textProperties
    ADD_USER_WRITER( TextProperties );
}
