// reserializing_write_test: validate the IR -> .osgt/.osgb writer by
// round-tripping an existing file:
//
//   read (IR) -> write -> read again -> compare the two IR summaries
//
// The comparison is structural (object/command histograms), so it stays stable
// across the id renumbering the writer performs.  The writer's warnings name
// every wrapper property that still lacks a write counterpart.
//
// Usage: reserializing_write_test <in.osgt|in.osgb> <out.osgt|out.osgb> [--version N]

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "GenericReader.h"
#include "GenericWriter.h"
#include "SceneIR.h"

#ifdef OSGVERSE_GENERIC_USE_STATIC_ACTIVATION
#include "GenericStaticWrappers.h"   // keep every wrapper registrar in a plain static link
#endif

namespace
{
    typedef std::map<std::string, unsigned int> Histogram;

    // Data volume carried by the IR; insensitive to legacy/modern command names.
    struct PayloadStats
    {
        unsigned long long vectorValues;
        unsigned long long vectorObjects;
        unsigned long long primitiveIndices;
        unsigned long long primitiveVertices;
        unsigned long long pixelBytes;
        unsigned long long fileImages;
        PayloadStats() : vectorValues(0), vectorObjects(0), primitiveIndices(0),
                         primitiveVertices(0), pixelBytes(0), fileImages(0) {}
    };

    void buildHistograms(const osgVerse::SceneIR& ir, Histogram& classes,
                         Histogram& commands, PayloadStats& stats)
    {
        for (size_t i = 0; i < ir.objects.size(); ++i)
        {
            const osgVerse::IRObject& o = ir.objects[i];
            ++classes[o.className];
            if (!o.vectorData.empty())
            {
                stats.vectorValues += o.vectorData.size();
                ++stats.vectorObjects;
            }
            stats.primitiveIndices += o.primIndices.size();
            stats.primitiveVertices += o.primCount;
            stats.pixelBytes += o.pixelData.size();
            if (!o.fileName.empty()) ++stats.fileImages;
        }
        for (size_t i = 0; i < ir.events.size(); ++i)
        {
            const osgVerse::IREvent& e = ir.events[i];
            if (e.type == osgVerse::IREvent::APPLY_COMMAND) ++commands[e.cmd.name];
        }
    }

    void printStats(const char* title, const PayloadStats& s)
    {
        std::cout << title << ": vector objects=" << s.vectorObjects
                  << " values=" << s.vectorValues
                  << " prim indices=" << s.primitiveIndices
                  << " prim vertices=" << s.primitiveVertices
                  << " pixel bytes=" << s.pixelBytes
                  << " external images=" << s.fileImages << std::endl;
    }

    void printHistogram(const char* title, const Histogram& h)
    {
        std::cout << "\n=== " << title << " ===" << std::endl;
        for (Histogram::const_iterator it = h.begin(); it != h.end(); ++it)
            std::cout << "  " << it->second << "  " << it->first << std::endl;
    }

    unsigned int countMissing(const Histogram& before, const Histogram& after)
    {
        unsigned int missing = 0;
        for (Histogram::const_iterator it = before.begin(); it != before.end(); ++it)
        {
            Histogram::const_iterator jt = after.find(it->first);
            unsigned int got = jt != after.end() ? jt->second : 0;
            if (got < it->second) missing += it->second - got;
        }
        return missing;
    }

    bool endsWith(const std::string& s, const std::string& suffix)
    {
        return s.size() >= suffix.size() &&
               s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Usage: reserializing_write_test <in.osgt|in.osgb> "
                     "<out.osgt|out.osgb> [--version N]" << std::endl;
        return 1;
    }

    const std::string input = argv[1];
    const std::string output = argv[2];
    int version = osgVerse::GENERIC_WRITER_DEFAULT_VERSION;
    for (int i = 3; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--version") version = atoi(argv[i + 1]);

    osgVerse::SceneIR source;
    if (!osgVerse::readSceneIR(input, source))
    {
        std::cerr << "Failed to read " << input << std::endl;
        return 1;
    }

    Histogram srcClasses, srcCommands;
    PayloadStats srcStats;
    buildHistograms(source, srcClasses, srcCommands, srcStats);
    std::cout << "Source      : " << input << std::endl;
    std::cout << "Objects     : " << source.objects.size() << std::endl;
    std::cout << "Events      : " << source.events.size() << std::endl;

    std::vector<std::string> warnings;
    const bool ascii = endsWith(output, ".osgt");

    // Write to memory first, so the same bytes can be written again after the
    // read-back and compared: identical output proves the reader/writer pair is
    // stable at the target file version (nothing is lost on a second pass).
    std::ostringstream firstStream;
    if (!osgVerse::writeObjectFile(firstStream, source, ascii, -1, version, &warnings))
    {
        std::cerr << "Failed to write " << output << std::endl;
        return 1;
    }
    {
        std::ofstream fout(output.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        fout.write(firstStream.str().data(), (std::streamsize)firstStream.str().size());
        if (!fout)
        {
            std::cerr << "Failed to save " << output << std::endl;
            return 1;
        }
    }
    std::cout << "Written     : " << output << " ("
              << (ascii ? "ASCII" : "binary") << ", osg version " << version
              << ", " << firstStream.str().size() << " bytes)" << std::endl;

    if (!warnings.empty())
    {
        std::cout << "\nWriter warnings (" << warnings.size() << "):" << std::endl;
        for (size_t i = 0; i < warnings.size(); ++i)
            std::cout << "  - " << warnings[i] << std::endl;
    }

    osgVerse::SceneIR result;
    if (!osgVerse::readSceneIR(output, result))
    {
        std::cerr << "Failed to read back " << output << std::endl;
        return 1;
    }

    std::vector<std::string> secondWarnings;
    std::ostringstream secondStream;
    const bool secondOk = osgVerse::writeObjectFile(secondStream, result, ascii, -1,
                                                    version, &secondWarnings);

    Histogram dstClasses, dstCommands;
    PayloadStats dstStats;
    buildHistograms(result, dstClasses, dstCommands, dstStats);
    std::cout << "\nRe-read objects : " << result.objects.size() << std::endl;
    std::cout << "Re-read events  : " << result.events.size() << std::endl;

    printHistogram("Source classes", srcClasses);
    printHistogram("Re-read classes", dstClasses);

    std::cout << "\nPayload comparison (informational):" << std::endl;
    printStats("  source ", srcStats);
    printStats("  re-read", dstStats);

    const unsigned int missingObjects = countMissing(srcClasses, dstClasses);
    const unsigned int missingCommands = countMissing(srcCommands, dstCommands);
    std::cout << "\nMissing objects  : " << missingObjects << std::endl;
    std::cout << "Command-name deltas (legacy setter spellings / version-gated fields): "
              << missingCommands << std::endl;
    if (missingCommands)
    {
        for (Histogram::const_iterator it = srcCommands.begin(); it != srcCommands.end(); ++it)
        {
            Histogram::const_iterator jt = dstCommands.find(it->first);
            unsigned int got = jt != dstCommands.end() ? jt->second : 0;
            if (got < it->second)
                std::cout << "  - " << it->first << " : " << it->second << " -> "
                          << got << std::endl;
        }
    }

    const bool stable = secondOk && firstStream.str() == secondStream.str();
    std::cout << "\nSecond write    : " << (secondOk ? "ok" : "failed")
              << ", " << secondStream.str().size() << " bytes; "
              << (stable ? "identical to the first write"
                         : "DIFFERS from the first write (unstable)")
              << std::endl;
    if (!stable)
    {
        const std::string againFile = output + ".again";
        std::ofstream fout(againFile.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        fout.write(secondStream.str().data(), (std::streamsize)secondStream.str().size());
        std::cout << "Second write saved for comparison: " << againFile << std::endl;
    }

    if (missingObjects || !warnings.empty() || !secondWarnings.empty() || !stable)
    {
        std::cout << "\nResult: LOSSY / UNSTABLE round-trip (see above)." << std::endl;
        return 2;
    }
    std::cout << "\nResult: OK (objects preserved; output is stable)." << std::endl;
    return 0;
}
