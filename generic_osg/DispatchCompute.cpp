#include <GenericReserializer.h>
using namespace osgVerse;

// _numGroupsX/Y/Z
static bool readComputeGroups(InputStream& is, InputUserData& ud)
{
    GLint numX = 0, numY = 0, numZ = 0;
    is >> numX >> numY >> numZ;
    ud.add("setComputeGroups", numX, numY, numZ);
    return true;
}

static bool writeComputeGroups(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("setComputeGroups");
    int numX = 0, numY = 0, numZ = 0;
    if (cmd && cmd->args.size() > 2)
    {
        numX = (int)irArgInt(cmd->args[0]); numY = (int)irArgInt(cmd->args[1]);
        numZ = (int)irArgInt(cmd->args[2]);
    }
    os << numX << numY << numZ << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( DispatchCompute,
                         new osg::DispatchCompute,
                         osg::DispatchCompute,
                         "osg::Object osg::Node osg::Drawable osg::DispatchCompute" )
{
        ADD_USER_SERIALIZER( ComputeGroups );  // _numGroupsX/Y/Z
        ADD_USER_WRITER( ComputeGroups );
}
