#include <GenericReserializer.h>
using namespace osgVerse;

namespace
{
    // Consumes one keyframe value: `components` scalars of 4 (float) or 8 (double)
    // bytes. The values are not needed by the visitor, only the stream position.
    void readKeyComponents(InputStream& is, unsigned int components, unsigned int scalarSize)
    {
        for (unsigned int c = 0; c < components; ++c)
        {
            if (scalarSize == 8) { double v; is >> v; }
            else { float v; is >> v; }
        }
    }
}

// _channels: each channel stores its type token, a name / target-name header and
// an optional keyframe container. The value layout depends on the type token.
static bool readChannels(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string type;
        is >> is.PROPERTY("Type") >> type >> is.BEGIN_BRACKET;

        std::string name, targetName;
        is >> is.PROPERTY("Name"); is.readWrappedString(name);
        is >> is.PROPERTY("TargetName"); is.readWrappedString(targetName);

        unsigned int components = 1, scalarSize = 4; bool cubic = false;
        if (type == "DoubleStepChannel" || type == "DoubleLinearChannel")
            scalarSize = 8;
        else if (type == "Vec2StepChannel" || type == "Vec2LinearChannel")
            components = 2;
        else if (type == "Vec3StepChannel" || type == "Vec3LinearChannel")
            components = 3;
        else if (type == "Vec4StepChannel" || type == "Vec4LinearChannel")
            components = 4;
        else if (type == "QuatStepChannel" || type == "QuatSphericalLinearChannel")
            components = 4;
        else if (type == "MatrixLinearChannel")
        { components = 16; scalarSize = 8; }
        else if (type == "FloatCubicBezierChannel")
            cubic = true;
        else if (type == "DoubleCubicBezierChannel")
        { cubic = true; scalarSize = 8; }
        else if (type == "Vec2CubicBezierChannel")
        { cubic = true; components = 2; }
        else if (type == "Vec3CubicBezierChannel")
        { cubic = true; components = 3; }
        else if (type == "Vec4CubicBezierChannel")
        { cubic = true; components = 4; }

        bool hasContainer = false;
        is >> is.PROPERTY("KeyFrameContainer") >> hasContainer;
        if (hasContainer)
        {
            unsigned int count = is.readSize(); is >> is.BEGIN_BRACKET;
            for (unsigned int k=0; k<count; ++k)
            {
                double time = 0.0; is >> time;
                readKeyComponents(is, components, scalarSize);
                if (cubic)
                {
                    readKeyComponents(is, components, scalarSize);  // control point in
                    readKeyComponents(is, components, scalarSize);  // control point out
                }
            }
            is >> is.END_BRACKET;
        }
        is >> is.END_BRACKET;

        ud.add("addChannel", type, name, targetName);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgAnimation_Animation,
                         new osgAnimation::Animation,
                         osgAnimation::Animation,
                         "osg::Object osgAnimation::Animation" )
{
    ADD_DOUBLE_SERIALIZER( Duration, 0.0f );  // _duration
    ADD_FLOAT_SERIALIZER( Weight, 0.0f );  // _weight
    ADD_DOUBLE_SERIALIZER( StartTime, 0.0f );  // _startTime

    BEGIN_ENUM_SERIALIZER( PlayMode, LOOP );
        ADD_ENUM_VALUE( ONCE );
        ADD_ENUM_VALUE( STAY );
        ADD_ENUM_VALUE( LOOP );
        ADD_ENUM_VALUE( PPONG );
    END_ENUM_SERIALIZER();  // _playmode

    ADD_USER_SERIALIZER( Channels );  // _channels
}
