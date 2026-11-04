#include "GenericWriter.h"
#include "GenericOutputStream.h"
#include "GenericReserializer.h"

#include <fstream>
#include <iostream>

namespace osgVerse
{

namespace
{
    int findRootObject(const SceneIR& ir)
    {
        for (size_t i = 0; i < ir.objects.size(); ++i)
            if (ir.objects[i].parent < 0) return (int)i;
        return ir.objects.empty() ? -1 : 0;
    }
}

bool writeObjectFile(std::ostream& fout, const SceneIR& ir, bool ascii,
                     int objectIndex, int version, std::vector<std::string>* warnings)
{
    if (objectIndex < 0) objectIndex = findRootObject(ir);
    if (objectIndex < 0 || objectIndex >= (int)ir.objects.size())
    {
        std::cerr << "[GenericWriter] Nothing to write: empty SceneIR." << std::endl;
        return false;
    }

    // Header prefix, written before the iterator (mirrors osg's plugin).
    if (ascii)
    {
        fout << "#Ascii ";
    }
    else
    {
        unsigned int low = OSG_HEADER_LOW, high = OSG_HEADER_HIGH;
        fout.write((const char*)&low, INT_SIZE);
        fout.write((const char*)&high, INT_SIZE);
    }

    osg::ref_ptr<OutputIterator> iterator;
    if (ascii) iterator = new AsciiOutputIterator(&fout);
    else iterator = new BinaryOutputIterator(&fout);

    OutputStream os;
    os.setFileVersion(version);

    const IRObject& root = ir.objects[objectIndex];
    OutputStream::WriteType type = (root.className == "osg::Image")
        ? OutputStream::WRITE_IMAGE : OutputStream::WRITE_SCENE;
    os.start(iterator.get(), type);

    OutputUserData ud(ir);
    if (root.className == "osg::Image") os.writeImage(ud, objectIndex);
    else os.writeObject(ud, objectIndex);

    if (warnings) *warnings = ud.getWarnings();

    if (!fout)
    {
        std::cerr << "[GenericWriter] Output stream failed." << std::endl;
        return false;
    }
    if (os.getException())
    {
        std::cerr << "[GenericWriter] Failed to write the scene (exception)." << std::endl;
        return false;
    }
    return true;
}

bool writeObjectFile(const std::string& filename, const SceneIR& ir, bool ascii,
                     int objectIndex, int version, std::vector<std::string>* warnings)
{
    std::ofstream fout(filename.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fout)
    {
        std::cerr << "[GenericWriter] Failed to open " << filename << " for writing." << std::endl;
        return false;
    }
    return writeObjectFile(fout, ir, ascii, objectIndex, version, warnings);
}

// ---- SceneIR-named convenience wrappers (mirror readSceneIR) ----
bool writeSceneIR(const std::string& filename, const SceneIR& ir,
                  bool ascii, int objectIndex)
{
    return writeObjectFile(filename, ir, ascii, objectIndex);
}

bool writeSceneIR(std::ostream& fout, const SceneIR& ir,
                  bool ascii, int objectIndex)
{
    return writeObjectFile(fout, ir, ascii, objectIndex);
}

}
