#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( TextSettings,
                         new osgUI::TextSettings,
                         osgUI::TextSettings,
                         "osg::Object osgUI::TextSettings" )
{
    ADD_STRING_SERIALIZER( Font, std::string());
    ADD_FLOAT_SERIALIZER( CharacterSize, 0.0f);
}
