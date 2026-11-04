#include <GenericReserializer.h>
using namespace osgVerse;

namespace DACommandsArrays {
REGISTER_OBJECT_WRAPPER( IndirectCommandDrawArrays,
                         0,
                         osg::IndirectCommandDrawArrays,
                         "osg::Object osg::BufferData osg::IndirectCommandDrawArrays" )
{
    {
        UPDATE_TO_VERSION_SCOPED( 147 )
        ADDED_ASSOCIATE("osg::BufferData")
    }
}
}
namespace DECommandsArrays {
REGISTER_OBJECT_WRAPPER( IndirectCommandDrawElements,
                         0,
                         osg::IndirectCommandDrawElements,
                         "osg::Object osg::BufferData osg::IndirectCommandDrawElements" )
{
    {
        UPDATE_TO_VERSION_SCOPED( 147 )
        ADDED_ASSOCIATE("osg::BufferData")
    }
}
}
namespace DefaultDACommandsArrays {

static bool readDACommands(InputStream& is, InputUserData& ud)
{
    unsigned int elmt, size = 0; is >> size >> is.BEGIN_BRACKET;
    ud.add("resize", size);
    for ( unsigned int i=0; i<size; ++i )
    {
        is >>elmt; ud.add("count", (i), elmt);
        is >>elmt; ud.add("instanceCount", (i), elmt);
        is >>elmt; ud.add("first", (i), elmt);
        is >>elmt; ud.add("baseInstance", (i), elmt);
    }
    is >> is.END_BRACKET;
    return true;
}

static unsigned int indexedValue(OutputUserData& ud, const char* name, unsigned int index)
{
    const IRCommand* cmd = ud.findCommandWithInt(name, index, 0);
    return (cmd && cmd->args.size() > 1) ? (unsigned int)irArgInt(cmd->args[1]) : 0;
}

static bool writeDACommands(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* resize = ud.findCommand("resize");
    unsigned int size = (resize && !resize->args.empty())
                      ? (unsigned int)irArgInt(resize->args[0]) : 0;

    os << size << os.BEGIN_BRACKET << std::endl;
    for ( unsigned int i=0; i<size; ++i )
    {
        os << indexedValue(ud, "count", i) << indexedValue(ud, "instanceCount", i)
           << indexedValue(ud, "first", i) << indexedValue(ud, "baseInstance", i) << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgDefaultIndirectCommandDrawArrays,
                         new osg::DefaultIndirectCommandDrawArrays,
                         osg::DefaultIndirectCommandDrawArrays,
                         "osg::Object osg::BufferData osg::IndirectCommandDrawArrays osg::DefaultIndirectCommandDrawArrays" )
{
    ADD_USER_SERIALIZER(DACommands);
    ADD_USER_WRITER(DACommands);
}
}
namespace DefaultDECommandsArrays {

static bool readDECommands(InputStream& is, InputUserData& ud)
{
    unsigned int elmt, size = 0; is >> size >> is.BEGIN_BRACKET;
    ud.add("resize", size);
    for ( unsigned int i=0; i<size; ++i )
    {
        is >>elmt; ud.add("count", (i), elmt);
        is >>elmt; ud.add("instanceCount", (i), elmt);
        is >>elmt; ud.add("firstIndex", (i), elmt);
        is >>elmt; ud.add("baseVertex", (i), elmt);
        is >>elmt; ud.add("baseInstance", (i), elmt);
    }
    is >> is.END_BRACKET;
    return true;
}

static unsigned int indexedValueDE(OutputUserData& ud, const char* name, unsigned int index)
{
    const IRCommand* cmd = ud.findCommandWithInt(name, index, 0);
    return (cmd && cmd->args.size() > 1) ? (unsigned int)irArgInt(cmd->args[1]) : 0;
}

static bool writeDECommands(OutputStream& os, OutputUserData& ud)
{
    const IRCommand* resize = ud.findCommand("resize");
    unsigned int size = (resize && !resize->args.empty())
                      ? (unsigned int)irArgInt(resize->args[0]) : 0;

    os << size << os.BEGIN_BRACKET << std::endl;
    for ( unsigned int i=0; i<size; ++i )
    {
        os << indexedValueDE(ud, "count", i) << indexedValueDE(ud, "instanceCount", i)
           << indexedValueDE(ud, "firstIndex", i) << indexedValueDE(ud, "baseVertex", i)
           << indexedValueDE(ud, "baseInstance", i) << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgDefaultIndirectCommandDrawElements,
                         new osg::DefaultIndirectCommandDrawElements,
                         osg::DefaultIndirectCommandDrawElements,
                         "osg::Object osg::BufferData osg::IndirectCommandDrawElements osg::DefaultIndirectCommandDrawElements" )
{
    {
        UPDATE_TO_VERSION_SCOPED( 147 )
        ADDED_ASSOCIATE("osg::BufferData")
    }
    ADD_USER_SERIALIZER(DECommands);
    ADD_USER_WRITER(DECommands);
}
}
namespace DrawArraysIndirectWrapper {

REGISTER_OBJECT_WRAPPER( DrawArraysIndirect,
                         new osg::DrawArraysIndirect,
                         osg::DrawArraysIndirect,
                         "osg::Object osg::BufferData osg::PrimitiveSet osg::DrawArraysIndirect" )
{
    ADD_OBJECT_SERIALIZER( IndirectCommandArray, osg::IndirectCommandDrawArrays, new osg::DefaultIndirectCommandDrawArrays());
    ADD_UINT_SERIALIZER( FirstCommandToDraw, 0);
    ADD_INT_SERIALIZER( Stride, 0);
}

}
namespace MultiDrawArraysIndirectWrapper {

REGISTER_OBJECT_WRAPPER( MultiDrawArraysIndirect,
                         new osg::MultiDrawArraysIndirect,
                         osg::MultiDrawArraysIndirect,
                         "osg::Object osg::BufferData osg::PrimitiveSet osg::DrawArraysIndirect osg::MultiDrawArraysIndirect" )
{
    ADD_UINT_SERIALIZER( NumCommandsToDraw, 0);
}

}

namespace DrawElementsIndirectWrapper {

REGISTER_OBJECT_WRAPPER( DrawElementsIndirect,
                         0,
                         osg::DrawElementsIndirect,
                         "osg::Object osg::BufferData osg::PrimitiveSet osg::DrawElementsIndirect" )
{
    ADD_OBJECT_SERIALIZER( IndirectCommandArray, osg::IndirectCommandDrawElements, new osg::DefaultIndirectCommandDrawElements());
    ADD_UINT_SERIALIZER( FirstCommandToDraw, 0);
    ADD_INT_SERIALIZER( Stride, 0);
}

}
#define INDIRECTDRAW_ELEMENTS_WRAPPER( DRAWELEMENTS, ELEMENTTYPE ) \
    namespace Wrapper##DRAWELEMENTS { \
        REGISTER_OBJECT_WRAPPER( DRAWELEMENTS, new osg::DRAWELEMENTS, osg::DRAWELEMENTS, "osg::Object osg::BufferData osg::PrimitiveSet osg::DrawElementsIndirect osg::"#DRAWELEMENTS) \
        { \
                ADD_ISAVECTOR_SERIALIZER( vector, osgDB::BaseSerializer::ELEMENTTYPE, 4 ); \
        } \
    }\
    namespace WrapperMulti##DRAWELEMENTS { \
        REGISTER_OBJECT_WRAPPER( Multi##DRAWELEMENTS, new osg::Multi##DRAWELEMENTS, osg::Multi##DRAWELEMENTS, "osg::Object osg::BufferData osg::PrimitiveSet osg::DrawElementsIndirect osg::"#DRAWELEMENTS" osg::Multi"#DRAWELEMENTS) \
        { \
    ADD_UINT_SERIALIZER( NumCommandsToDraw, 0);\
        } \
    }\


INDIRECTDRAW_ELEMENTS_WRAPPER( DrawElementsIndirectUByte, RW_UCHAR )
INDIRECTDRAW_ELEMENTS_WRAPPER( DrawElementsIndirectUShort, RW_USHORT )
INDIRECTDRAW_ELEMENTS_WRAPPER( DrawElementsIndirectUInt, RW_UINT )


