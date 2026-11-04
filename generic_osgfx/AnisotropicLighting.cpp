#include <GenericReserializer.h>
using namespace osgVerse;

static bool readLightingMap(InputStream& is, InputUserData& ud)
{
    std::string fileName; is.readWrappedString( fileName );
    ud.add("setLightingMap", fileName);
    return true;
}

static bool writeLightingMap(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setLightingMap");
    os.writeWrappedString((cmd && !cmd->args.empty()) ? irArgString(cmd->args[0])
                                                      : std::string());
    os << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgFX_AnisotropicLighting,
                         new osgFX::AnisotropicLighting,
                         osgFX::AnisotropicLighting,
                         "osg::Object osg::Node osg::Group osgFX::Effect osgFX::AnisotropicLighting" )
{
    ADD_INT_SERIALIZER( LightNumber, 0 );  // _lightnum
    ADD_USER_SERIALIZER( LightingMap );  // _texture
    ADD_USER_WRITER( LightingMap );
}
