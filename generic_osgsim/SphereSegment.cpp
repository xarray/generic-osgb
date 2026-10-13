#include <GenericReserializer.h>
using namespace osgVerse;

static bool readArea(InputStream& is, InputUserData& ud)
{
    float azMin, azMax, elevMin, elevMax;
    is >> azMin >> azMax >> elevMin >> elevMax;
    ud.add("setArea",  azMin, azMax, elevMin, elevMax );
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_SphereSegment,
                         new osgSim::SphereSegment,
                         osgSim::SphereSegment,
                         "osg::Object osg::Node osg::Geode osgSim::SphereSegment" )
{
    ADD_VEC3_SERIALIZER( Centre, osg::Vec3() );  // _centre
    ADD_FLOAT_SERIALIZER( Radius, 1.0f );  // _radius
    ADD_USER_SERIALIZER( Area );  // _azMin, _azMax, _elevMin, _elevMax
    ADD_INT_SERIALIZER( Density, 10 );  // _density
    ADD_INT_SERIALIZER( DrawMask, 0x7fffffff );  // _drawMask (osgSim::SphereSegment::ALL)
    ADD_VEC4_SERIALIZER( SurfaceColor, osg::Vec4() );  // _surfaceColor
    ADD_VEC4_SERIALIZER( SpokeColor, osg::Vec4() );  // _spokeColor
    ADD_VEC4_SERIALIZER( EdgeLineColor, osg::Vec4() );  // _edgeLineColor
    ADD_VEC4_SERIALIZER( SideColor, osg::Vec4() );  // _planeColor
}
