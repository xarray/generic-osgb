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
