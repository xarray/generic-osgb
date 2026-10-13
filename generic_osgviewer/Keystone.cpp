#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( osgViewer_Keystone,
                         new osgViewer::Keystone,
                         osgViewer::Keystone,
                         "osg::Object osgViewer::Keystone" )
{
    ADD_BOOL_SERIALIZER( KeystoneEditingEnabled, true );
    ADD_VEC4_SERIALIZER( GridColor, osg::Vec4(1.0,1.0,1.0,1.0) );
    ADD_VEC2D_SERIALIZER( BottomLeft, osg::Vec2d(0.0,0.0) );
    ADD_VEC2D_SERIALIZER( BottomRight, osg::Vec2d(0.0,0.0) );
    ADD_VEC2D_SERIALIZER( TopLeft, osg::Vec2d(0.0,0.0) );
    ADD_VEC2D_SERIALIZER( TopRight, osg::Vec2d(0.0,0.0) );
}
