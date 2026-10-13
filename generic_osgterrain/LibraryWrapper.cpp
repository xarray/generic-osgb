// LibraryWrapper: the mount point for the osgTerrain wrappers.
//
// Each *_Wrapper.cpp above registers statically through REGISTER_OBJECT_WRAPPER,
// so nothing has to be pulled in explicitly: linking the archive in full
// (whole-archive) runs every registrar. This symbol only documents / anchors the
// library, mirroring the upstream osgTerrain serializers layout.

extern "C" void rewrapper_serializer_library_osgTerrain(void) {}
