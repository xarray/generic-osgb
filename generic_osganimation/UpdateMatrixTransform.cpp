#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;

static bool readStackedTransforms(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID element = ud.readObjectFromStream(is, "osgAnimation::StackedTransformElement");
        if ( element ) ud.add("addStackedTransform", i, &element);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgAnimation_UpdateMatrixTransform,
                         new osgAnimation::UpdateMatrixTransform,
                         osgAnimation::UpdateMatrixTransform,
                         "osg::Object osg::Callback osg::NodeCallback osgAnimation::UpdateMatrixTransform" )
{
    ADD_USER_SERIALIZER( StackedTransforms );  // _transforms
}

#undef OBJECT_CAST
#define OBJECT_CAST static_cast
