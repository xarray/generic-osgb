// image_decode.cpp -- JPEG/PNG decoding for the OpenGL example.
//
// stb_image is compiled into this translation unit (no external codec library),
// so the example only needs OpenGL + a window toolkit. The .osgb files store
// most textures as an embedded encoded file (osg's "inline file" decision),
// which is JPEG in the sample databases.
//
// stb_image returns top-down rows; they are flipped so the result matches osg's
// bottom-up order. The GL texture is then uploaded verbatim and v = 0 samples
// the bottom of the picture, as osg expects.

#include "image_decode.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#include "stb_image.h"

namespace gl_ir
{

namespace
{

void flipRows(std::vector<unsigned char>& rgba, int width, int height)
{
    const size_t rowBytes = (size_t)width * 4;
    std::vector<unsigned char> tmp(rowBytes);
    for (int y = 0; y < height / 2; ++y)
    {
        unsigned char* a = &rgba[(size_t)y * rowBytes];
        unsigned char* b = &rgba[(size_t)(height - 1 - y) * rowBytes];
        std::memcpy(&tmp[0], a, rowBytes);
        std::memcpy(a, b, rowBytes);
        std::memcpy(b, &tmp[0], rowBytes);
    }
}

}  // namespace

bool decodeImageMemory(const unsigned char* data, size_t size, const std::string& hint,
                       ImageRGBA& out)
{
    (void)hint;   // stb_image detects JPEG/PNG from the data itself
    out = ImageRGBA();
    if (!data || size == 0) return false;

    int width = 0, height = 0, components = 0;
    unsigned char* pixels = stbi_load_from_memory(data, (int)size, &width, &height,
                                                  &components, 4);
    if (!pixels || width <= 0 || height <= 0)
    {
        if (pixels) stbi_image_free(pixels);
        return false;
    }

    out.width = width;
    out.height = height;
    out.pixels.assign(pixels, pixels + (size_t)width * height * 4);
    stbi_image_free(pixels);

    flipRows(out.pixels, width, height);
    return true;
}

bool decodeImageFile(const std::string& filename, ImageRGBA& out)
{
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in) return false;

    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),
                                     std::istreambuf_iterator<char>());
    if (bytes.empty()) return false;
    return decodeImageMemory(&bytes[0], bytes.size(), filename, out);
}

}  // namespace gl_ir
