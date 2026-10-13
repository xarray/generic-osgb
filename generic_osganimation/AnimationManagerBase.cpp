#undef OBJECT_CAST
#define OBJECT_CAST dynamic_cast

#include <GenericReserializer.h>
using namespace osgVerse;
namespace  osgAnimation_AnimationManagerBaseWrapper
{

static bool readAnimations(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize();
    is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID ani = ud.readObjectFromStream(is, "osgAnimation::Animation");
        if ( ani ) ud.add("registerAnimation",  ani.get() );
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgAnimation_AnimationManagerBase,
                         /*new osgAnimation::AnimationManagerBase*/NULL,
                         osgAnimation::AnimationManagerBase,
                         "osg::Object osg::NodeCallback osgAnimation::AnimationManagerBase" )
{
    ADD_USER_SERIALIZER( Animations );  // _animations
    ADD_BOOL_SERIALIZER( AutomaticLink, true );  // _automaticLink
    
    {
        UPDATE_TO_VERSION_SCOPED( 152 )

    }
}
}
#undef OBJECT_CAST
#define OBJECT_CAST static_cast
