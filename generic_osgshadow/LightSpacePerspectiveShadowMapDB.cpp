#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgShadow_LightSpacePerspectiveShadowMapDB,
                         new osgShadow::LightSpacePerspectiveShadowMapDB,
                         osgShadow::LightSpacePerspectiveShadowMapDB,
                         "osg::Object osgShadow::ShadowTechnique osgShadow::ViewDependentShadowTechnique "
                         "osgShadow::DebugShadowMap osgShadow::StandardShadowMap osgShadow::MinimalShadowMap "
                         "osgShadow::MinimalDrawBoundsShadowMap osgShadow::LightSpacePerspectiveShadowMapDB" )
{
}
