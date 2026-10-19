// null_integration: the zero-dependency reference integration.
//
// It reads an .osgt/.osgb file into the neutral SceneIR and prints what the IR
// describes (scene tree, class/command histograms, data payload summaries).
// There is no rendering engine here, so this file depends only on the generic
// reader and SceneIR.h -- a template for how a real engine adapter consumes the
// IR through ISceneBuilder.
//
// Usage: null_integration <file.osgt|file.osgb>

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <cstdio>

#include "SceneIR.h"

#ifdef OSGVERSE_GENERIC_USE_STATIC_ACTIVATION
#include "GenericStaticWrappers.h"   // keep every wrapper registrar in a plain static link
#endif

namespace
{
    const char* payloadSummary(const osgVerse::IRObject& o, std::string& out)
    {
        char buf[160];
        if (!o.vectorData.empty() && o.vectorComponents > 0)
        {
            unsigned int elements = (unsigned int)(o.vectorData.size() / (size_t)o.vectorComponents);
            std::string token = o.vectorToken.empty() ? std::string() : (" " + o.vectorToken);
            std::snprintf(buf, sizeof(buf), " [array: %u x %d%s]",
                          elements, o.vectorComponents, token.c_str());
            out = buf;
        }
        else if (!o.primIndices.empty())
        {
            std::snprintf(buf, sizeof(buf), " [draw: mode=%d, %u indices]",
                          o.primMode, (unsigned int)o.primIndices.size());
            out = buf;
        }
        else if (o.primCount != 0)
        {
            std::snprintf(buf, sizeof(buf), " [draw: mode=%d, first=%u, count=%u]",
                          o.primMode, o.primFirst, o.primCount);
            out = buf;
        }
        else if (!o.fileName.empty())
            out = " [image: " + o.fileName + "]";
        else if (!o.pixelData.empty())
        {
            std::snprintf(buf, sizeof(buf), " [image: %dx%dx%d, %u bytes]",
                          o.width, o.height, o.depth, (unsigned int)o.pixelData.size());
            out = buf;
        }
        else if (o.internalFormat != 0 || o.pixelFormat != 0)
        {
            std::snprintf(buf, sizeof(buf), " [image: %dx%dx%d]",
                          o.width, o.height, o.depth);
            out = buf;
        }
        else
            out.clear();
        return out.c_str();
    }

    // A no-op engine: it just records statistics while the IR is replayed.
    class NullBuilder : public osgVerse::ISceneBuilder
    {
    public:
        void onObject(const osgVerse::IRObject& object, int index) override
        { ++_classCounts[object.className]; }
        void onCommand(const osgVerse::IRObject& object, int index,
                       const osgVerse::IRCommand& command) override
        { ++_commandCounts[command.name]; }

        const std::map<std::string, unsigned int>& classCounts() const { return _classCounts; }
        const std::map<std::string, unsigned int>& commandCounts() const { return _commandCounts; }

    protected:
        std::map<std::string, unsigned int> _classCounts;
        std::map<std::string, unsigned int> _commandCounts;
    };

    void printTree(const osgVerse::SceneIR& ir, const std::vector<std::vector<int> >& children,
                   int idx, int depth)
    {
        const osgVerse::IRObject& o = ir.objects[idx];
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::string extra;
        std::cout << "- " << o.className;
        if (!o.id.empty()) std::cout << " (id=" << o.id << ")";
        std::cout << payloadSummary(o, extra) << std::endl;
        for (size_t i = 0; i < children[idx].size(); ++i)
            printTree(ir, children, children[idx][i], depth + 1);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: null_integration <file.osgt|file.osgb>" << std::endl;
        return 1;
    }

    osgVerse::SceneIR ir;
    if (!osgVerse::readSceneIR(argv[1], ir))
    {
        std::cerr << "Failed to read " << argv[1] << std::endl;
        return 1;
    }

    // Replay the IR into the no-op builder to exercise the engine-facing path.
    NullBuilder builder;
    osgVerse::buildScene(ir, builder);

    std::cout << "=== IR summary ===" << std::endl;
    std::cout << "Objects : " << ir.objects.size() << std::endl;
    std::cout << "Events  : " << ir.events.size() << std::endl;

    std::cout << "\n=== Objects by class ===" << std::endl;
    for (std::map<std::string, unsigned int>::const_iterator it = builder.classCounts().begin();
         it != builder.classCounts().end(); ++it)
        std::cout << "  " << it->second << "  " << it->first << std::endl;

    std::cout << "\n=== Commands / properties ===" << std::endl;
    for (std::map<std::string, unsigned int>::const_iterator it = builder.commandCounts().begin();
         it != builder.commandCounts().end(); ++it)
        std::cout << "  " << it->second << "  " << it->first << std::endl;

    std::cout << "\n=== Scene tree ===" << std::endl;
    std::vector<std::vector<int> > children(ir.objects.size());
    std::vector<int> roots;
    for (size_t i = 0; i < ir.objects.size(); ++i)
    {
        int p = ir.objects[i].parent;
        if (p >= 0 && p < (int)ir.objects.size()) children[p].push_back((int)i);
        else roots.push_back((int)i);
    }
    for (size_t i = 0; i < roots.size(); ++i)
        printTree(ir, children, roots[i], 0);

    std::cout << "Done." << std::endl;
    return 0;
}
