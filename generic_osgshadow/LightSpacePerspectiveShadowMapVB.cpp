#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgShadow_LightSpacePerspectiveShadowMapVB,
                         new osgShadow::LightSpacePerspectiveShadowMapVB,
                         osgShadow::LightSpacePerspectiveShadowMapVB,
                         "osg::Object osgShadow::ShadowTechnique osgShadow::ViewDependentShadowTechnique "
                         "osgShadow::DebugShadowMap osgShadow::StandardShadowMap osgShadow::MinimalShadowMap "
                         "osgShadow::LightSpacePerspectiveShadowMapVB" )
{
}
