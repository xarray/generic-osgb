#include <GenericReserializer.h>
using namespace osgVerse;

static bool readSelectedTechnique(InputStream& is, InputUserData& ud)
{
    int sel = 0; is >> sel;
    ud.add("selectTechnique",  sel );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgFX_Effect,
                         /*new osgFX::Effect*/NULL,
                         osgFX::Effect,
                         "osg::Object osg::Node osg::Group osgFX::Effect" )
{
    ADD_BOOL_SERIALIZER( Enabled, true );  // _enabled
    ADD_USER_SERIALIZER( SelectedTechnique );  // _global_sel_tech
}
