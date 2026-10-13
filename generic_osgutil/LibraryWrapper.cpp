#include <GenericReserializer.h>
using namespace osgVerse;

USE_SERIALIZER_WRAPPER(CullVisitor)
USE_SERIALIZER_WRAPPER(UpdateVisitor)

extern "C" void wrapper_serializer_library_osgUtil(void) {}
