// reserializing_test: verify the .osgt/.osgb read capability of generic_wrappers (zero osg dependency).
//
// Usage: reserializing_test <file.osgt|file.osgb> [--force-ascii]

#include <iostream>
#include <string>
#include <vector>

#include "GenericReader.h"

namespace
{
    // Print the object scene tree recorded during reading.
    // Garbled class names (or negative-looking offsets) pinpoint stream misalignment
    // immediately: the node's startPos field gives the exact file offset to inspect.
    void printTree(const std::vector<osgVerse::InputUserData::TreeNode>& nodes,
                   int idx, int depth)
    {
        const size_t maxChildrenShown = 20;
        const osgVerse::InputUserData::TreeNode& n = nodes[idx];
        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << "- " << n.className << " (id=" << n.id;
        if (n.startPos != std::streampos(0))
            std::cout << ", @0x" << std::hex << (long long)n.startPos << std::dec;
        std::cout << ")" << std::endl;

        size_t shown = 0;
        for (size_t i = 0; i < n.children.size(); ++i)
        {
            if (shown >= maxChildrenShown)
            {
                for (int j = 0; j < depth + 1; ++j) std::cout << "  ";
                std::cout << "... " << (n.children.size() - shown) << " more children" << std::endl;
                break;
            }
            printTree(nodes, n.children[i], depth + 1);
            ++shown;
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: reserializing_test <file.osgt|file.osgb> [--force-ascii]"
                  << std::endl;
        return 1;
    }

    const std::string filename = argv[1];
    bool forceAscii = false;
    if (argc > 2 && std::string(argv[2]) == "--force-ascii") forceAscii = true;

    if (!osgVerse::loadRewrappers())
    {
        std::cerr << "No rewrappers registered -- nothing linked?" << std::endl;
        return 1;
    }

    std::cout << "Rewrappers: " <<
        osgVerse::RewrapperManager::instance()->getRewrappers().size() << std::endl;

    try
    {
        osgVerse::InputUserData ud;
        osgVerse::ObjectTypeAndID oid =
            osgVerse::readObjectFile(filename, ud, forceAscii);
        if (!oid.valid())
        {
            std::cerr << "Failed to read " << filename << std::endl;
            return 1;
        }

        std::cout << "Top object  : " << oid._type << " (id=" << oid._id << ")" << std::endl;

        // Scene-tree dump (for misalignment diagnostics)
        {
            const auto& nodes = ud.getTreeNodes();
            std::cout << "Scene tree  : " << nodes.size() << " objects read" << std::endl;
            for (size_t i = 0; i < nodes.size(); ++i)
                if (nodes[i].parentId < 0) printTree(nodes, (int)i, 0);
        }

        std::cout << "Done." << std::endl;
    }
    catch (const osgVerse::InputException& e)
    {
        std::cerr << "InputException: " << e.getMessage() << std::endl;
        return 2;
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 2;
    }
    catch (...)
    {
        std::cerr << "UNKNOWN EXCEPTION" << std::endl;
        return 2;
    }
    return 0;
}
