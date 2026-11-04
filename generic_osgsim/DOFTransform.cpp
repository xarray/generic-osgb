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

static bool writePutMatrix(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setPutMatrix");
    osg::Matrixf put;
    if (cmd && !cmd->args.empty())
    {
        for ( int i=0; i<16; ++i ) put._m[i] = (float)cmd->args[0].v[i];
    }
    os << put << std::endl;
    return true;
}

static bool writeLimitationFlags(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setLimitationFlags");
    unsigned long flags = (cmd && !cmd->args.empty()) ? (unsigned long)irArgInt(cmd->args[0]) : 0;
    os << flags << std::endl;
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
    ADD_USER_WRITER( PutMatrix );
    ADD_USER_SERIALIZER( LimitationFlags );  // _limitationFlags
    ADD_USER_WRITER( LimitationFlags );
    ADD_BOOL_SERIALIZER( AnimationOn, false );  // _animationOn

    BEGIN_ENUM_SERIALIZER2( HPRMultOrder, osgSim::DOFTransform::MultOrder, PRH );
        ADD_ENUM_VALUE2( PRH, 0 );
        ADD_ENUM_VALUE2( PHR, 1 );
        ADD_ENUM_VALUE2( HPR, 2 );
        ADD_ENUM_VALUE2( HRP, 3 );
        ADD_ENUM_VALUE2( RPH, 4 );
        ADD_ENUM_VALUE2( RHP, 5 );
    END_ENUM_SERIALIZER();  // _multOrder
}
