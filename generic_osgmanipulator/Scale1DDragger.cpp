#include <GenericReserializer.h>
using namespace osgVerse;

#define HANDLENODE_FUNC( PROP ) \
    static bool read##PROP( InputStream& is, InputUserData& ud ) { \
        ObjectTypeAndID node = ud.readObjectFromStream(is, "osg::Node"); \
        if ( node ) ud.add( "set" #PROP, &node ); return true; \
    }

HANDLENODE_FUNC( LeftHandleNode )
HANDLENODE_FUNC( RightHandleNode )

REGISTER_OBJECT_WRAPPER( osgManipulator_Scale1DDragger,
                         new osgManipulator::Scale1DDragger,
                         osgManipulator::Scale1DDragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger "
                         "osgManipulator::Scale1DDragger" )
{
    ADD_DOUBLE_SERIALIZER( MinScale, 0.0 );// _minScale
    ADD_VEC4_SERIALIZER( Color, osg::Vec4() );  // _color
    ADD_VEC4_SERIALIZER( PickColor, osg::Vec4() );  // _pickColor
    ADD_USER_SERIALIZER( LeftHandleNode );  // _leftHandleNode
    ADD_USER_SERIALIZER( RightHandleNode );  // _rightHandleNode
    ADD_DOUBLE_SERIALIZER( LeftHandlePosition, 0.0 );
    ADD_DOUBLE_SERIALIZER( RightHandlePosition, 0.0 );
}
