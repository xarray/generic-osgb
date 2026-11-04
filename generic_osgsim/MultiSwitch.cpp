#include <GenericReserializer.h>
using namespace osgVerse;

static bool readValues(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("SwitchSet");
        unsigned int valueSize = is.readSize(); is >> is.BEGIN_BRACKET;
        ud.add("setValueList", i, valueSize);

        for ( unsigned int j=0; j<valueSize; ++j )
        {
            bool value = false; is >> value;
            ud.add("setValue", i, j, value);
        }
        is >> is.END_BRACKET;
    }
    is >> is.END_BRACKET;
    return true;
}

static bool writeValues(OutputStream& os, OutputUserData& ud)
{
    std::vector<const IRCommand*> lists;
    ud.findCommands("setValueList", lists);

    unsigned int count = 0;
    for ( size_t i=0; i<lists.size(); ++i )
    {
        if (!lists[i]->args.empty())
            count = std::max(count, (unsigned int)irArgInt(lists[i]->args[0]) + 1);
    }

    os << count << os.BEGIN_BRACKET << std::endl;
    for ( unsigned int i=0; i<count; ++i )
    {
        unsigned int valueSize = 0;
        for ( size_t k=0; k<lists.size(); ++k )
        {
            if (!lists[k]->args.empty() &&
                (unsigned int)irArgInt(lists[k]->args[0]) == i && lists[k]->args.size() > 1)
            { valueSize = (unsigned int)irArgInt(lists[k]->args[1]); break; }
        }

        os << os.PROPERTY("SwitchSet") << valueSize << os.BEGIN_BRACKET << std::endl;
        std::vector<const IRCommand*> values;
        ud.findCommands("setValue", values);
        for ( unsigned int j=0; j<valueSize; ++j )
        {
            bool v = false;
            for ( size_t k=0; k<values.size(); ++k )
            {
                const IRCommand& c = *values[k];
                if (c.args.size() > 2 && (unsigned int)irArgInt(c.args[0]) == i &&
                    (unsigned int)irArgInt(c.args[1]) == j)
                { v = irArgBool(c.args[2]); break; }
            }
            os << v << std::endl;
        }
        os << os.END_BRACKET << std::endl;
    }
    os << os.END_BRACKET << std::endl;
    return true;
}

REGISTER_OBJECT_WRAPPER( osgSim_MultiSwitch,
                         new osgSim::MultiSwitch,
                         osgSim::MultiSwitch,
                         "osg::Object osg::Node osg::Group osgSim::MultiSwitch" )
{
    ADD_BOOL_SERIALIZER( NewChildDefaultValue, true );  // _newChildDefaultValue
    ADD_UINT_SERIALIZER( ActiveSwitchSet, 0 );  // _activeSwitchSet
    ADD_USER_SERIALIZER( Values );  // _values
    ADD_USER_WRITER( Values );
}
