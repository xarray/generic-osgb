#pragma once

// GenericWriter: zero-osg-dependency entry point for writing .osgt/.osgb files
// from a SceneIR (the neutral object graph produced by the generic reader, or
// built by a host application).
//
// Usage:
//   osgVerse::SceneIR ir;
//   osgVerse::readSceneIR("in.osgb", ir);
//   osgVerse::writeObjectFile("out.osgt", ir, true);   // ASCII
//   osgVerse::writeObjectFile("out.osgb", ir, false);  // binary
//
// The writer targets osg file version 161 by default (arrays and primitive
// sets are full objects, as osg >= 112 writes them).  Missing wrappers or
// missing user-serializer writers do not abort the write: as much of the IR as
// can be represented is emitted and the optional `warnings` vector reports the
// gaps (handy while porting the remaining write functions).

#include <iosfwd>
#include <string>
#include <vector>

#include "SceneIR.h"

namespace osgVerse
{

// Matches OpenSceneGraph's current SOVERSION in this tree (161).
const int GENERIC_WRITER_DEFAULT_VERSION = 161;

// Write the scene rooted at `objectIndex` (default: the first parentless
// object).  `ascii` selects .osgt text output; otherwise the raw .osgb layout
// is written (uncompressed, byte order of the host).
bool writeObjectFile(const std::string& filename, const SceneIR& ir,
                     bool ascii = false, int objectIndex = -1,
                     int version = GENERIC_WRITER_DEFAULT_VERSION,
                     std::vector<std::string>* warnings = NULL);

// Same, to an already-open stream positioned at the beginning of the output.
bool writeObjectFile(std::ostream& fout, const SceneIR& ir,
                     bool ascii = false, int objectIndex = -1,
                     int version = GENERIC_WRITER_DEFAULT_VERSION,
                     std::vector<std::string>* warnings = NULL);

}
