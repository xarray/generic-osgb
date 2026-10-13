#ifndef OSGVERSE_GENERIC_STATIC_WRAPPERS_H
#define OSGVERSE_GENERIC_STATIC_WRAPPERS_H

// Static-archive activation.
//
// Normally the wrapper registrars are kept alive by a whole-archive link
// (-Wl,--whole-archive / /WHOLEARCHIVE), so an executable never mentions them.
// When the core is linked as a plain static archive
// (cmake -DENABLE_STATIC_LIBRARY=ON) the linker would drop them instead; this
// header supplies the missing references. Include it once in a translation unit
// of the final executable that is always linked -- the examples guard the
// include with OSGVERSE_GENERIC_USE_STATIC_ACTIVATION, which
// ENABLE_STATIC_LIBRARY defines for them.
//
// Mirrors osgDB's USE_SERIALIZER_WRAPPER_LIBRARY(): one anchor per ported
// library, each of which keeps that library's LibraryWrapper.cpp member, whose
// own USE_SERIALIZER_WRAPPER() references keep every per-class registrar.
//
// The header is deliberately self-contained: it must stay includable from a
// translation unit that also includes the real osg headers (e.g.
// examples/osg_integration.cpp), so it must not pull in the mini-osg math layer
// of GenericReserializer.h.
namespace osgVerseStaticWrappers
{
    // Referencing the anchor from a static initializer makes it a used symbol.
    struct LibraryAnchor
    {
        explicit LibraryAnchor(void (*fn)()) { if (fn) fn(); }
    };
}

#define USE_SERIALIZER_WRAPPER_LIBRARY(libname) \
    extern "C" void wrapper_serializer_library_##libname(void); \
    [[maybe_unused]] static osgVerseStaticWrappers::LibraryAnchor \
        wrapper_library_anchor_##libname(&wrapper_serializer_library_##libname);

USE_SERIALIZER_WRAPPER_LIBRARY(osg)
USE_SERIALIZER_WRAPPER_LIBRARY(osgAnimation)
USE_SERIALIZER_WRAPPER_LIBRARY(osgFX)
USE_SERIALIZER_WRAPPER_LIBRARY(osgGA)
USE_SERIALIZER_WRAPPER_LIBRARY(osgGUI)
USE_SERIALIZER_WRAPPER_LIBRARY(osgManipulator)
USE_SERIALIZER_WRAPPER_LIBRARY(osgParticle)
USE_SERIALIZER_WRAPPER_LIBRARY(osgShadow)
USE_SERIALIZER_WRAPPER_LIBRARY(osgSim)
USE_SERIALIZER_WRAPPER_LIBRARY(osgTerrain)
USE_SERIALIZER_WRAPPER_LIBRARY(osgText)
USE_SERIALIZER_WRAPPER_LIBRARY(osgUtil)
USE_SERIALIZER_WRAPPER_LIBRARY(osgViewer)
USE_SERIALIZER_WRAPPER_LIBRARY(osgVolume)

#endif
