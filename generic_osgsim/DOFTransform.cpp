#include <GenericReserializer.h>
using namespace osgVerse;

static bool readPutMatrix(InputStream& is, InputUserData& ud)
{
    osg::Matrixf put; is >> put;
    ud.add("setPutMatrix", put);
    return true;
}

static bool readLimitationFlags(InputStream& is, InputUserData& ud)
{
    unsigned long flags; is >> flags;
    ud.add("setLimitationFlags",  flags );;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_DOFTransform,
                         new osgSim::DOFTransform,
                         osgSim::DOFTransform,
                         "osg::Object osg::Node osg::Group osg::Transform osgSim::DOFTransform" )
{
    ADD_VEC3_SERIALIZER( MinHPR, osg::Vec3() );  // _minHPR
    ADD_VEC3_SERIALIZER( MaxHPR, osg::Vec3() );  // _maxHPR
    ADD_VEC3_SERIALIZER( CurrentHPR, osg::Vec3() );  // _currentHPR
    ADD_VEC3_SERIALIZER( IncrementHPR, osg::Vec3() );  // _incrementHPR
    ADD_VEC3_SERIALIZER( MinTranslate, osg::Vec3() );  // _minTranslate
    ADD_VEC3_SERIALIZER( MaxTranslate, osg::Vec3() );  // _maxTranslate
    ADD_VEC3_SERIALIZER( CurrentTranslate, osg::Vec3() );  // _currentTranslate
    ADD_VEC3_SERIALIZER( IncrementTranslate, osg::Vec3() );  // _incrementTranslate
    ADD_VEC3_SERIALIZER( MinScale, osg::Vec3() );  // _minScale
    ADD_VEC3_SERIALIZER( MaxScale, osg::Vec3() );  // _maxScale
    ADD_VEC3_SERIALIZER( CurrentScale, osg::Vec3() );  // _currentScale
    ADD_VEC3_SERIALIZER( IncrementScale, osg::Vec3() );  // _incrementScale
    ADD_USER_SERIALIZER( PutMatrix );  // _Put, _inversePut
    ADD_USER_SERIALIZER( LimitationFlags );  // _limitationFlags
    ADD_BOOL_SERIALIZER( AnimationOn, false );  // _animationOn

    BEGIN_ENUM_SERIALIZER2( HPRMultOrder, osgSim::DOFTransform::MultOrder, PRH );
        ADD_ENUM_VALUE( PRH );
        ADD_ENUM_VALUE( PHR );
        ADD_ENUM_VALUE( HPR );
        ADD_ENUM_VALUE( HRP );
        ADD_ENUM_VALUE( RPH );
        ADD_ENUM_VALUE( RHP );
    END_ENUM_SERIALIZER();  // _multOrder
}
