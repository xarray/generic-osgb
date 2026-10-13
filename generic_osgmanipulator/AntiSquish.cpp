#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgManipulator_AntiSquish,
                         new osgManipulator::AntiSquish,
                         osgManipulator::AntiSquish,
                         "osg::Object osg::Node osg::Group osg::Transform osg::MatrixTransform osgManipulator::AntiSquish" )
{
    ADD_VEC3D_SERIALIZER( Pivot, osg::Vec3d() );  // _pivot
    ADD_VEC3D_SERIALIZER( Position, osg::Vec3d() );  // _position
}
