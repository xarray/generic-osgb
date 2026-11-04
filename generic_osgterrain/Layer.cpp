#include <GenericReserializer.h>
using namespace osgVerse;

// osgTerrain::Layer::MAXIMUM_NUMBER_OF_LEVELS (kept as a plain literal here, as
// this layer must not depend on any osg/osgTerrain header).
#define MAXIMUM_NUMBER_OF_LEVELS 30

// _validDataOperator (a NoDataValue / ValidRange depending on the type tag)
static bool readValidDataOperator(InputStream& is, InputUserData& ud)
{
    unsigned int type = 0; is >> type;
    switch ( type )
    {
    case 1:
        {
            float value; is >> value;
            ud.add( "setValidDataOperator", (unsigned int)1, value );
        }
        break;
    case 2:
        {
            float min, max; is >> min >> max;
            ud.add( "setValidDataOperator", (unsigned int)2, min, max );
        }
        break;
    default:
        break;
    }
    return true;
}

static bool writeValidDataOperator(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setValidDataOperator");
    unsigned int type = 0;
    if (cmd && !cmd->args.empty()) type = (unsigned int)irArgInt(cmd->args[0]);

    os << type;
    if ( type == 1 && cmd && cmd->args.size() > 1 )
        os << (float)irArgDouble(cmd->args[1]);
    else if ( type == 2 && cmd && cmd->args.size() > 2 )
        os << (float)irArgDouble(cmd->args[1]) << (float)irArgDouble(cmd->args[2]);
    if ( !os.isBinary() ) os << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_Layer,
                         new osgTerrain::Layer,
                         osgTerrain::Layer,
                         "osg::Object osgTerrain::Layer" )
{
    ADD_STRING_SERIALIZER( FileName, "" );  // _filename
    ADD_OBJECT_SERIALIZER( Locator, osgTerrain::Locator, NULL );  // _locator
    ADD_UINT_SERIALIZER( MinLevel, 0 );  // _minLevel
    ADD_UINT_SERIALIZER( MaxLevel, MAXIMUM_NUMBER_OF_LEVELS );  // _maxLevel
    ADD_USER_SERIALIZER( ValidDataOperator );  // _validDataOperator
    ADD_USER_WRITER( ValidDataOperator );
    ADD_VEC4_SERIALIZER( DefaultValue, osg::Vec4() );  // _defaultValue
    ADD_GLENUM_SERIALIZER( MinFilter, osg::Texture::FilterMode, GL_LINEAR_MIPMAP_LINEAR );  // _minFilter
    ADD_GLENUM_SERIALIZER( MagFilter, osg::Texture::FilterMode, GL_LINEAR );  // _magFilter
}
