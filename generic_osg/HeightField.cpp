#include <GenericReserializer.h>
using namespace osgVerse;

// _columns, _rows
static bool readArea(InputStream& is, InputUserData& ud)
{
    unsigned int numCols, numRows;
    is >> numCols >> numRows;
    ud.add("allocate", numCols, numRows);
    return true;
}

// _heights
static bool readHeights(InputStream& is, InputUserData& ud)
{
    ObjectTypeAndID farray = ud.readObjectFromStream(is, "osg::FloatArray");
    if ( farray.valid() )
    {
        unsigned int numCols = 0, numRows = 0; ud.get("allocate", &numCols, &numRows);
        unsigned int fsize = 0; ud.get(farray, "size", &fsize);
        if ( fsize < numRows*numCols ) return false;

        unsigned int index = 0;
        for ( unsigned int r=0; r<numRows; ++r )
        {
            for (unsigned int c = 0; c < numCols; ++c)
            {
                float v = 0.0f; ud.get(farray, "index", &index, &v);
                ud.add("setHeight", c, r, v); index++;
            }
        }
    }
    return true;
}

static bool writeArea(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* cmd = ud.findCommand("allocate");
    unsigned int numCols = 0, numRows = 0;
    if (cmd && cmd->args.size() > 1)
    {
        numCols = (unsigned int)irArgInt(cmd->args[0]);
        numRows = (unsigned int)irArgInt(cmd->args[1]);
    }
    os << numCols << numRows << std::endl;
    return true;
}

static bool writeHeights(OutputStream& os, OutputUserData& ud)
{
    // The height values are serialized as a separate osg::FloatArray object,
    // which readHeights() created as a child of this HeightField.
    int cur = ud.getCurrentObject(), target = -1;
    const std::vector<IRObject>& objs = ud.getObjects();
    for (size_t i = 0; i < objs.size(); ++i)
    {
        if (objs[i].parent == cur && objs[i].className == "osg::FloatArray")
        { target = (int)i; break; }
    }

    if (os.isBinary())
    {
        os << (target >= 0);
        if (target >= 0) os.writeObject(ud, target);
    }
    else if (target >= 0)
        os.writeObject(ud, target);
    return true;
}

REGISTER_OBJECT_WRAPPER( HeightField,
                         new osg::HeightField,
                         osg::HeightField,
                         "osg::Object osg::Shape osg::HeightField" )
{
    ADD_USER_SERIALIZER( Area );  // _columns, _rows
    ADD_USER_WRITER( Area );
    ADD_VEC3_SERIALIZER( Origin, osg::Vec3() );  // _origin
    ADD_FLOAT_SERIALIZER( XInterval, 0.0f );  // _dx
    ADD_FLOAT_SERIALIZER( YInterval, 0.0f );  // _dy
    ADD_FLOAT_SERIALIZER( SkirtHeight, 0.0f );  // _skirtHeight
    ADD_UINT_SERIALIZER( BorderWidth, 0 );  // _borderWidth
    ADD_QUAT_SERIALIZER( Rotation, osg::Quat() );  // _rotation
    ADD_USER_SERIALIZER( Heights );  // _heights
    ADD_USER_WRITER( Heights );
}
