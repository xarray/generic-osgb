#include <GenericReserializer.h>
using namespace osgVerse;

// osgParticle::Particle::Shape values (the generic layer has no osg headers).
enum osgParticle_Particle_Shape { POINT, QUAD, QUAD_TRIANGLESTRIP, HEXAGON, LINE, USER };

BEGIN_USER_TABLE( Shape, osgParticle::Particle );
    ADD_USER_VALUE( POINT );
    ADD_USER_VALUE( QUAD );
    ADD_USER_VALUE( QUAD_TRIANGLESTRIP );
    ADD_USER_VALUE( HEXAGON );
    ADD_USER_VALUE( LINE );
    ADD_USER_VALUE( USER );
END_USER_TABLE()

USER_READ_FUNC( Shape, readShapeValue )

// Reads one osgParticle::Particle and forwards its fields as a single command.
bool readParticle( InputStream& is, InputUserData& ud )
{
    is >> is.BEGIN_BRACKET;

    is >> is.PROPERTY("Shape");
    int shape = readShapeValue(is);

    double lifeTime = 0.0; is >> is.PROPERTY("LifeTime") >> lifeTime;

    float sizeMin = 0.0f, sizeMax = 0.0f, alphaMin = 0.0f, alphaMax = 0.0f;
    osg::Vec4d colorMin, colorMax;
    is >> is.PROPERTY("SizeRange") >> sizeMin >> sizeMax;
    is >> is.PROPERTY("AlphaRange") >> alphaMin >> alphaMax;
    is >> is.PROPERTY("ColorRange") >> colorMin >> colorMax;

    ObjectTypeAndID sizeInterpolator, alphaInterpolator, colorInterpolator;
    bool hasInterpolator = false;
    is >> is.PROPERTY("SizeInterpolator") >> hasInterpolator;
    if ( hasInterpolator )
    {
        is >> is.BEGIN_BRACKET;
        sizeInterpolator = ud.readObjectFromStream(is, "osgParticle::Interpolator");
        is >> is.END_BRACKET;
    }
    is >> is.PROPERTY("AlphaInterpolator") >> hasInterpolator;
    if ( hasInterpolator )
    {
        is >> is.BEGIN_BRACKET;
        alphaInterpolator = ud.readObjectFromStream(is, "osgParticle::Interpolator");
        is >> is.END_BRACKET;
    }
    is >> is.PROPERTY("ColorInterpolator") >> hasInterpolator;
    if ( hasInterpolator )
    {
        is >> is.BEGIN_BRACKET;
        colorInterpolator = ud.readObjectFromStream(is, "osgParticle::Interpolator");
        is >> is.END_BRACKET;
    }

    float radius = 0.0f, mass = 0.0f;
    osg::Vec3d pos, vel, angle, angleV;
    is >> is.PROPERTY("Radius") >> radius;
    is >> is.PROPERTY("Mass") >> mass;
    is >> is.PROPERTY("Position") >> pos;
    is >> is.PROPERTY("Velocity") >> vel;
    is >> is.PROPERTY("Angle") >> angle;
    is >> is.PROPERTY("AngularVelocity") >> angleV;
    int s = 0, t = 0, num = 0; is >> is.PROPERTY("TextureTile") >> s >> t >> num;

    ud.add("setParticle", shape, lifeTime, sizeMin, sizeMax, alphaMin, alphaMax,
           colorMin, colorMax, radius, mass, pos, vel, angle, angleV, s, t, num,
           &sizeInterpolator, &alphaInterpolator, &colorInterpolator);

    if (is.getFileVersion()<145)
    {
        bool hasObject = false; is >> is.PROPERTY("Drawable") >> hasObject;
        if ( hasObject )
        {
            is >> is.BEGIN_BRACKET;
            ud.readObjectFromStream(is, "osg::Drawable");
            is >> is.END_BRACKET;
        }
    }
    is >> is.END_BRACKET;
    return true;
}

// Writes one osgParticle::Particle block from the "setParticle" command.
bool writeParticle( OutputStream& os, OutputUserData& ud )
{
    const IRCommand* cmd = ud.findCommand("setParticle");
    const std::vector<IRArg>* a = cmd ? &cmd->args : NULL;
    const IRArg none;
    #define PARTICLE_ARG(I) ((a && a->size() > (I)) ? (*a)[I] : none)

    os << os.BEGIN_BRACKET << std::endl;

    int shape = (int)irArgInt(PARTICLE_ARG(0));
    if (os.isBinary()) os << shape;
    else
    {
        const std::string& token = UserLookupTableProxy::get("Shape").value(shape);
        os << os.PROPERTY("Shape")
           << (token.empty() ? std::to_string(shape) : token) << std::endl;
    }

    if (!os.isBinary()) os << os.PROPERTY("LifeTime");
    os << irArgDouble(PARTICLE_ARG(1));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("SizeRange");
    os << (float)irArgDouble(PARTICLE_ARG(2)) << (float)irArgDouble(PARTICLE_ARG(3));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("AlphaRange");
    os << (float)irArgDouble(PARTICLE_ARG(4)) << (float)irArgDouble(PARTICLE_ARG(5));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("ColorRange");
    os << irArgVec4d(PARTICLE_ARG(6)) << irArgVec4d(PARTICLE_ARG(7));
    if (!os.isBinary()) os << std::endl;

    const char* interpolators[3] = { "SizeInterpolator", "AlphaInterpolator", "ColorInterpolator" };
    for (int i = 0; i < 3; ++i)
    {
        int target = ud.resolve(PARTICLE_ARG(17 + i));
        if (!os.isBinary()) os << os.PROPERTY(interpolators[i]);
        os << (target >= 0);
        if (target >= 0)
        {
            os << os.BEGIN_BRACKET << std::endl;
            if (os.isBinary()) os << true;   // readObjectFromStream's own flag
            os.writeObject(ud, target);
            os << os.END_BRACKET << std::endl;
        }
        else if (!os.isBinary()) os << std::endl;
    }

    if (!os.isBinary()) os << os.PROPERTY("Radius");
    os << (float)irArgDouble(PARTICLE_ARG(8));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("Mass");
    os << (float)irArgDouble(PARTICLE_ARG(9));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("Position");
    os << irArgVec3d(PARTICLE_ARG(10));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("Velocity");
    os << irArgVec3d(PARTICLE_ARG(11));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("Angle");
    os << irArgVec3d(PARTICLE_ARG(12));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("AngularVelocity");
    os << irArgVec3d(PARTICLE_ARG(13));
    if (!os.isBinary()) os << std::endl;

    if (!os.isBinary()) os << os.PROPERTY("TextureTile");
    os << (int)irArgInt(PARTICLE_ARG(14)) << (int)irArgInt(PARTICLE_ARG(15))
       << (int)irArgInt(PARTICLE_ARG(16));
    if (!os.isBinary()) os << std::endl;

    if (os.getFileVersion() < 145)
    {
        if (!os.isBinary()) os << os.PROPERTY("Drawable");
        os << false;
        if (!os.isBinary()) os << std::endl;
    }

    os << os.END_BRACKET << std::endl;
    #undef PARTICLE_ARG
    return true;
}
