#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( Transform,
                         new osg::Transform,
                         osg::Transform,
                         "osg::Object osg::Node osg::Group osg::Transform" )
{
    BEGIN_ENUM_SERIALIZER( ReferenceFrame, RELATIVE_RF );
        ADD_ENUM_VALUE2( RELATIVE_RF, 0 );
        ADD_ENUM_VALUE2( ABSOLUTE_RF, 1 );
        ADD_ENUM_VALUE2( ABSOLUTE_RF_INHERIT_VIEWPOINT, 2 );
    END_ENUM_SERIALIZER();  // _referenceFrame
}
