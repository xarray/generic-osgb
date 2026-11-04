#include <GenericReserializer.h>
using namespace osgVerse;

namespace
{
    // Channel layout description, shared by the read and write sides.
    struct ChannelLayout
    {
        unsigned int components;
        unsigned int scalarSize;
        bool cubic;
        ChannelLayout() : components(1), scalarSize(4), cubic(false) {}
    };

    ChannelLayout channelLayoutOf(const std::string& type)
    {
        ChannelLayout l;
        if (type == "DoubleStepChannel" || type == "DoubleLinearChannel")
            l.scalarSize = 8;
        else if (type == "Vec2StepChannel" || type == "Vec2LinearChannel")
            l.components = 2;
        else if (type == "Vec3StepChannel" || type == "Vec3LinearChannel")
            l.components = 3;
        else if (type == "Vec4StepChannel" || type == "Vec4LinearChannel")
            l.components = 4;
        else if (type == "QuatStepChannel" || type == "QuatSphericalLinearChannel")
            l.components = 4;
        else if (type == "MatrixLinearChannel")
        { l.components = 16; l.scalarSize = 8; }
        else if (type == "FloatCubicBezierChannel")
            l.cubic = true;
        else if (type == "DoubleCubicBezierChannel")
        { l.cubic = true; l.scalarSize = 8; }
        else if (type == "Vec2CubicBezierChannel")
        { l.cubic = true; l.components = 2; }
        else if (type == "Vec3CubicBezierChannel")
        { l.cubic = true; l.components = 3; }
        else if (type == "Vec4CubicBezierChannel")
        { l.cubic = true; l.components = 4; }
        return l;
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

        ChannelLayout layout = channelLayoutOf(type);

        bool hasContainer = false;
        is >> is.PROPERTY("KeyFrameContainer") >> hasContainer;
        if (hasContainer)
        {
            unsigned int count = is.readSize(); is >> is.BEGIN_BRACKET;
            for (unsigned int k=0; k<count; ++k)
            {
                double time = 0.0; is >> time;
                ud.add("addKeyTime", (unsigned int)i, (unsigned int)k, time);

                unsigned int total = layout.components * (layout.cubic ? 3u : 1u);
                for (unsigned int c=0; c<total; ++c)
                {
                    double v = 0.0;
                    if (layout.scalarSize == 8) { double d; is >> d; v = d; }
                    else { float f; is >> f; v = f; }
                    ud.add("addKeyValue", (unsigned int)i, (unsigned int)k,
                           (unsigned int)c, v);
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

static bool writeChannels(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> channels, times, values;
    ud.findCommands("addChannel", channels);
    ud.findCommands("addKeyTime", times);
    ud.findCommands("addKeyValue", values);

    os << (unsigned int)channels.size() << os.BEGIN_BRACKET << std::endl;
    for ( size_t i=0; i<channels.size(); ++i )
    {
        const IRCommand& ch = *channels[i];
        std::string type = ch.args.size() > 0 ? irArgString(ch.args[0]) : std::string();
        std::string name = ch.args.size() > 1 ? irArgString(ch.args[1]) : std::string();
        std::string targetName = ch.args.size() > 2 ? irArgString(ch.args[2]) : std::string();
        ChannelLayout layout = channelLayoutOf(type);

        os << os.PROPERTY("Type") << type << os.BEGIN_BRACKET << std::endl;
        os << os.PROPERTY("Name"); os.writeWrappedString(name); os << std::endl;
        os << os.PROPERTY("TargetName"); os.writeWrappedString(targetName); os << std::endl;

        unsigned int keyCount = 0;
        for ( size_t k=0; k<times.size(); ++k )
        {
            const IRCommand& c = *times[k];
            if (c.args.size() > 1 && (unsigned int)irArgInt(c.args[0]) == i)
                keyCount = std::max(keyCount, (unsigned int)irArgInt(c.args[1]) + 1);
        }

        os << os.PROPERTY("KeyFrameContainer") << (keyCount > 0);
        if ( keyCount > 0 )
        {
            os << keyCount << os.BEGIN_BRACKET << std::endl;
            unsigned int total = layout.components * (layout.cubic ? 3u : 1u);
            for ( unsigned int k=0; k<keyCount; ++k )
            {
                double time = 0.0;
                for ( size_t j=0; j<times.size(); ++j )
                {
                    const IRCommand& c = *times[j];
                    if (c.args.size() > 2 && (unsigned int)irArgInt(c.args[0]) == i &&
                        (unsigned int)irArgInt(c.args[1]) == k)
                    { time = irArgDouble(c.args[2]); break; }
                }
                os << time;

                for ( unsigned int e=0; e<total; ++e )
                {
                    double v = 0.0;
                    for ( size_t j=0; j<values.size(); ++j )
                    {
                        const IRCommand& c = *values[j];
                        if (c.args.size() > 3 && (unsigned int)irArgInt(c.args[0]) == i &&
                            (unsigned int)irArgInt(c.args[1]) == k &&
                            (unsigned int)irArgInt(c.args[2]) == e)
                        { v = irArgDouble(c.args[3]); break; }
                    }
                    if (layout.scalarSize == 8) os << v;
                    else os << (float)v;
                }
                os << std::endl;
            }
            os << os.END_BRACKET << std::endl;
        }
        else
            os << std::endl;
        os << os.END_BRACKET << std::endl;
    }
    os << os.END_BRACKET << std::endl;
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
        ADD_ENUM_VALUE2( ONCE, 0 );
        ADD_ENUM_VALUE2( STAY, 1 );
        ADD_ENUM_VALUE2( LOOP, 2 );
        ADD_ENUM_VALUE2( PPONG, 3 );
    END_ENUM_SERIALIZER();  // _playmode

    ADD_USER_SERIALIZER( Channels );  // _channels
    ADD_USER_WRITER( Channels );
}
