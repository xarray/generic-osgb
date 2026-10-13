#include <GenericReserializer.h>
using namespace osgVerse;

#define HANDLENODE_FUNC( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        ObjectTypeAndID node = ud.readObjectFromStream(is, "osg::Node"); \
        if ( node ) ud.add( "set" #PROP, &node ); return true; \
    }

HANDLENODE_FUNC( TopLeftHandleNode )
HANDLENODE_FUNC( BottomLeftHandleNode )
HANDLENODE_FUNC( TopRightHandleNode )
HANDLENODE_FUNC( BottomRightHandleNode )

REGISTER_OBJECT_WRAPPER( osgManipulator_Scale2DDragger,
                         new osgManipulator::Scale2DDragger,
                         osgManipulator::Scale2DDragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger "
                         "osgManipulator::Scale2DDragger" )
{
    ADD_VEC2D_SERIALIZER( MinScale, osg::Vec2d() );// _minScale
    ADD_VEC4_SERIALIZER( Color, osg::Vec4() );  // _color
    ADD_VEC4_SERIALIZER( PickColor, osg::Vec4() );  // _pickColor
    ADD_USER_SERIALIZER( TopLeftHandleNode );  // _topLeftHandleNode
    ADD_USER_SERIALIZER( BottomLeftHandleNode );  // _bottomLeftHandleNode
    ADD_USER_SERIALIZER( TopRightHandleNode );  // _topRightHandleNode
    ADD_USER_SERIALIZER( BottomRightHandleNode );  // _bottomRightHandleNode
    ADD_VEC2D_SERIALIZER( TopLeftHandlePosition, osg::Vec2d() );
    ADD_VEC2D_SERIALIZER( BottomLeftHandlePosition, osg::Vec2d() );
    ADD_VEC2D_SERIALIZER( TopRightHandlePosition, osg::Vec2d() );
    ADD_VEC2D_SERIALIZER( BottomRightHandlePosition, osg::Vec2d() );
}
