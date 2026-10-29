#ifndef OSGVERSE_GENERIC_WRAPPERS_IMAGE_DECODE_H
#define OSGVERSE_GENERIC_WRAPPERS_IMAGE_DECODE_H

#include <cstddef>
#include <string>
#include <vector>

namespace gl_ir
{

// Decoded 8-bit RGBA image in osg's row order: the first row in `pixels` is the
// bottom row of the picture, matching osg's v = 0 at the bottom.
struct ImageRGBA
{
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;

    bool valid() const
    { return width > 0 && height > 0 && pixels.size() == (size_t)width * height * 4; }
};

// Decode a JPEG or PNG from memory (signature sniffing; `hint` is only used when
// the signature is unknown, e.g. ".jpg"/".png").
bool decodeImageMemory(const unsigned char* data, size_t size, const std::string& hint,
                       ImageRGBA& out);

// Decode a JPEG/PNG file from disk.
bool decodeImageFile(const std::string& filename, ImageRGBA& out);

}  // namespace gl_ir

#endif
