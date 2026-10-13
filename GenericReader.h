#pragma once

// GenericReader: zero-osg-dependency entry point for reading .osgt/.osgb files.
//
// Usage:
//   osgVerse::loadRewrappers();
//   std::ifstream fin(filename, std::ios::binary);
//   osgVerse::InputUserData ud;
//   osgVerse::ObjectTypeAndID oid = osgVerse::readObjectFile(fin, ud);
//   // oid._type is the top-level object class name, e.g. "osg::Group";
//   // empty on failure.

#include <istream>
#include <string>

#include "GenericInputStream.h"
#include "GenericReserializer.h"

namespace osgVerse
{

// Ensure all rewrappers are registered. The static registrations in each
// generic_osg/*.cpp happen automatically before main runs;
// this function is only a safety net: returning false means no rewrapper is
// currently available.
bool loadRewrappers();

// Identify the format from the file header and create the matching input iterator:
//   - First 8 bytes are OSG_HEADER -> BinaryInputIterator
//   - Text starting with #Ascii   -> AsciiInputIterator
// The returned iterator must be deleted by the caller (wrapping in ref_ptr is safer).
InputIterator* readInputIterator(std::istream& fin);

// Read the top-level object of an .osgt/.osgb file. The filename overload
// opens the file stream internally; when forceAscii is true the binary
// detection is skipped and the file is parsed as ASCII.
ObjectTypeAndID readObjectFile(const std::string& filename, InputUserData& ud,
                               bool forceAscii = false);

// Read from an already-open input stream. The caller must have positioned
// fin at the start of the file before invocation.
ObjectTypeAndID readObjectFile(std::istream& fin, InputUserData& ud,
                               bool forceAscii = false);

}
