#include <GenericReserializer.h>
using namespace osgVerse;

// _filterMatrix (an osg::Matrix3, i.e. 9 floats written row by row)
static bool readFilterMatrix(InputStream& is, InputUserData& ud)
{
    is >> is.BEGIN_BRACKET;
    float m[9];
    for ( int r = 0; r < 3; ++r )
        is >> m[r * 3 + 0] >> m[r * 3 + 1] >> m[r * 3 + 2];
    is >> is.END_BRACKET;
    ud.add( "setFilterMatrix", m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8] );
    return true;
}

static bool writeFilterMatrix(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setFilterMatrix");
    float m[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    if (cmd)
    {
        for (size_t i = 0; i < cmd->args.size() && i < 9; ++i)
            m[i] = (float)irArgDouble(cmd->args[i]);
    }

    os << os.BEGIN_BRACKET << std::endl;
    for ( int r = 0; r < 3; ++r )
        os << m[r * 3 + 0] << m[r * 3 + 1] << m[r * 3 + 2] << std::endl;
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgTerrain_GeometryTechnique,
                         new osgTerrain::GeometryTechnique,
                         osgTerrain::GeometryTechnique,
                         "osg::Object osgTerrain::TerrainTechnique osgTerrain::GeometryTechnique" )
{
    ADD_FLOAT_SERIALIZER( FilterBias, 0.0f );  // _filterBias
    ADD_FLOAT_SERIALIZER( FilterWidth, 0.1f );  // _filterWidth
    ADD_USER_SERIALIZER( FilterMatrix );  // _filterMatrix
    ADD_USER_WRITER( FilterMatrix );
}
