#include <GenericReserializer.h>
using namespace osgVerse;

static bool readDraggers(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID child = ud.readObjectFromStream(is, "osgManipulator::Dragger");
        if ( child ) ud.add("addDragger",  child );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgManipulator_CompositeDragger,
                         /*new osgManipulator::CompositeDragger*/NULL,
                         osgManipulator::CompositeDragger,
                         "osg::Object osg::Node osg::Transform osg::MatrixTransform osgManipulator::Dragger "
                         "osgManipulator::CompositeDragger" )
{
    ADD_USER_SERIALIZER( Draggers );  // _draggerList
}
