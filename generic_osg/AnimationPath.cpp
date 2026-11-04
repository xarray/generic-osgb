#include <GenericReserializer.h>
using namespace osgVerse;

static bool readTimeControlPointMap(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize();
    if ( size>0 )
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            double time = 0.0;
            osg::Vec3d pos, scale;
            osg::Quat rot;
            is >> is.PROPERTY("Time") >> time >> is.BEGIN_BRACKET;
            is >> is.PROPERTY("Position") >> pos;
            is >> is.PROPERTY("Rotation") >> rot;
            is >> is.PROPERTY("Scale") >> scale;
            is >> is.END_BRACKET;
            ud.add("readTimeControlPointMap", time, pos, rot, scale);
        }
        is >> is.END_BRACKET;
    }
    return true;
}

static bool writeTimeControlPointMap(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> cmds;
    ud.findCommands("readTimeControlPointMap", cmds);

    os << (unsigned int)cmds.size();
    if (!cmds.empty())
    {
        os << os.BEGIN_BRACKET << std::endl;
        for (size_t i = 0; i < cmds.size(); ++i)
        {
            const IRCommand& c = *cmds[i];
            double time = c.args.size() > 0 ? irArgDouble(c.args[0]) : 0.0;
            osg::Vec3d pos, scale(1.0, 1.0, 1.0);
            osg::Quat rot;
            if (c.args.size() > 1) pos = irArgVec3d(c.args[1]);
            if (c.args.size() > 2) rot = irArgQuat(c.args[2]);
            if (c.args.size() > 3) scale = irArgVec3d(c.args[3]);

            os << os.PROPERTY("Time") << time << os.BEGIN_BRACKET << std::endl;
            os << os.PROPERTY("Position") << pos << std::endl;
            os << os.PROPERTY("Rotation") << rot << std::endl;
            os << os.PROPERTY("Scale") << scale << std::endl;
            os << os.END_BRACKET << std::endl;
        }
        os << os.END_BRACKET;
    }
    os << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( AnimationPath,
                         new osg::AnimationPath,
                         osg::AnimationPath,
                         "osg::Object osg::AnimationPath" )
{
    ADD_USER_SERIALIZER( TimeControlPointMap );  // _timeControlPointMap
    ADD_USER_WRITER( TimeControlPointMap );

    BEGIN_ENUM_SERIALIZER( LoopMode, LOOP );
        ADD_ENUM_VALUE2( SWING, 0 );
        ADD_ENUM_VALUE2( LOOP, 1 );
        ADD_ENUM_VALUE2( NO_LOOPING, 2 );
    END_ENUM_SERIALIZER();  //_loopMode
}
