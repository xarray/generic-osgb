#include "GenericReader.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <mutex>

// OSG_HEADER_LOW / OSG_HEADER_HIGH constants are provided by GenericInputStream.h (const unsigned int)

namespace osgVerse
{

bool loadRewrappers()
{
    // Each generic_osg/*.cpp registers statically via REGISTER_OBJECT_WRAPPER;
    // once linked into the executable the registration must already be done.
    // Here we only check that the manager is non-empty.
    return RewrapperManager::instance() != NULL &&
           !RewrapperManager::instance()->getRewrappers().empty();
}

InputIterator* readInputIterator(std::istream& fin)
{
    // Read the first 8 bytes to detect the binary magic
    std::streampos orig = fin.tellg();
    char header[8];
    fin.read(header, 8);
    if (fin.gcount() == 8)
    {
        unsigned int low = 0, high = 0;
        std::memcpy(&low, header, 4);
        std::memcpy(&high, header + 4, 4);
        if (low == OSG_HEADER_LOW && high == OSG_HEADER_HIGH)
        {
            fin.seekg(orig);
            return new BinaryInputIterator(&fin);
        }
    }

    // Text format: #Ascii ... (or legacy #... header)
    fin.seekg(orig);
    return new AsciiInputIterator(&fin);
}

ObjectTypeAndID readObjectFile(std::istream& fin, InputUserData& ud, bool forceAscii)
{
    // The Rewrapper/BaseSerializer objects are registered once and shared by every
    // read; they carry mutable scratch buffers (`_v0`, `_values`, `_elements`,
    // `_valueMap`, ...) that a traversal writes to. Concurrent traversals - e.g.
    // several files pulled in at once by a paging reader such as VSG's
    // DatabasePager - would corrupt those buffers (and the heap), so the wrapper
    // traversal is serialized through this single choke point. Each traversal is
    // non-recursive with respect to this function, so a plain mutex suffices.
    static std::mutex s_readMutex;
    std::lock_guard<std::mutex> lock(s_readMutex);

    ObjectTypeAndID result;
    result._type.clear();
    result._id.clear();

    InputIterator* iterator = NULL;
    if (forceAscii)
        iterator = new AsciiInputIterator(&fin);
    else
        iterator = readInputIterator(fin);
    if (!iterator) return result;

    osg::ref_ptr<InputIterator> iteratorRef(iterator);
    InputStream is;
    is.start(iterator);
    if (is.getException()) return result;

    is.decompress();
    if (is.getException()) return result;

    return is.readObject(ud);
}

ObjectTypeAndID readObjectFile(const std::string& filename, InputUserData& ud, bool forceAscii)
{
    std::ifstream fin(filename.c_str(), std::ios::in | std::ios::binary);
    if (!fin)
    {
        std::cerr << "[GenericReader] Failed to open " << filename << std::endl;
        return ObjectTypeAndID();
    }
    return readObjectFile(fin, ud, forceAscii);
}

}
