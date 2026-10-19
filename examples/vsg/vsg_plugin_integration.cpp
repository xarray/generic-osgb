// vsg_plugin_integration: dynamic paging of osgb/osgt databases in VulkanSceneGraph.
//
// One of OSG's real strengths is that a PagedLOD/ProxyNode lets a small root file
// name its children, which the viewer then pulls in on demand. VSG has the same
// model (vsg::PagedLOD + vsg::DatabasePager) -- it just needs a ReaderWriter for
// the osg file formats. This example provides one:
//
//   OsgbReaderWriter : vsg::ReaderWriter
//       .osgb/.osgt -> generic_wrappers reader -> neutral SceneIR
//                   -> shared VsgSceneBuilder (see vsg_scene_builder.h)
//
// so the whole chain becomes:
//
//   osg::PagedLOD / osg::ProxyNode
//        -> vsg::PagedLOD{ filename = child .osgb, children[1] = in-memory fallback }
//        -> VSG DatabasePager: vsg::read(filename, plod->options)
//        -> OsgbReaderWriter::read() -> readSceneIR() -> VsgSceneBuilder
//        -> merged back in as children[0]
//
// The in-memory fallback is what you see first; the pager then attaches the next
// level as soon as its bounding sphere occupies enough of the screen (--lod-ratio).
// --load-levels N does the same thing synchronously for headless verification.
//
// Build: -DENABLE_VSG=ON -DVSG_ROOT=<sdk>
// Usage: vsg_plugin_integration <root.osgb> [--list] [--load-levels N]
//                                [--lod-ratio R] [--max-paged-lod N]
//                                [--screenshot <image>] [-f <frames>]

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "SceneIR.h"
#include "vsg_scene_builder.h"

#ifdef OSGVERSE_GENERIC_USE_STATIC_ACTIVATION
#include "GenericStaticWrappers.h"   // keep every wrapper registrar in a plain static link
#endif

namespace
{
    vsg::Path directoryOf(const vsg::Path& path)
    {
        const std::string::size_type pos = path.find_last_of("/\\");
        return (pos == std::string::npos) ? vsg::Path(".") : vsg::Path(path.substr(0, pos));
    }

    // Counts the vsg::PagedLOD nodes rebuilt from osg::PagedLOD / osg::ProxyNode.
    // (VSG's Visitor::apply() does not descend, so traverse explicitly.)
    struct PagedLODCounter : public vsg::Visitor
    {
        unsigned int count = 0;
        void apply(vsg::Node& node) override
        {
            if (node.cast<vsg::PagedLOD>()) ++count;
            node.traverse(*this);
        }
    };

    // Walk the rebuilt scene graph and synchronously pull in N levels of external
    // PagedLOD children -- exactly what the DatabasePager does at runtime, but
    // deterministic and bounded so it can be verified without a display.
    void preloadPagedLOD(vsg::Node* node, int depth, int maxDepth, int& loaded)
    {
        if (!node || depth > maxDepth) return;

        if (vsg::PagedLOD* plod = node->cast<vsg::PagedLOD>())
        {
            if (!plod->filename.empty() && !plod->children[0].node)
            {
                vsg::ref_ptr<vsg::Object> object = vsg::read(plod->filename, plod->options);
                vsg::ref_ptr<vsg::Node> child = object.cast<vsg::Node>();
                if (!child)
                {
                    std::cerr << "  failed to load " << plod->filename << std::endl;
                    return;
                }
                plod->children[0].node = child;
                ++loaded;
                std::cout << "  [" << depth << "] " << plod->filename << std::endl;
            }
            if (plod->children[0].node) preloadPagedLOD(plod->children[0].node.get(), depth + 1, maxDepth, loaded);
            if (plod->children[1].node) preloadPagedLOD(plod->children[1].node.get(), depth, maxDepth, loaded);
            return;
        }
        if (vsg::Group* g = node->cast<vsg::Group>())
        {
            for (auto& c : g->children) preloadPagedLOD(c.get(), depth, maxDepth, loaded);
            return;
        }
        if (vsg::LOD* lod = node->cast<vsg::LOD>())
        {
            for (auto& c : lod->children) preloadPagedLOD(c.node.get(), depth, maxDepth, loaded);
            return;
        }
        if (vsg::Switch* sw = node->cast<vsg::Switch>())
        {
            for (auto& c : sw->children) preloadPagedLOD(c.node.get(), depth, maxDepth, loaded);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// The plugin
// ---------------------------------------------------------------------------
class OsgbReaderWriter : public vsg::Inherit<vsg::ReaderWriter, OsgbReaderWriter>
{
public:
    // minimumScreenHeightRatio for a PagedLOD's external child (0 = always visible).
    double pagedLodRatio = 0.25;

    // Diagnostics: the child file names discovered per file while reading. Filled
    // for every read (root and paged children), so it ends up describing the whole
    // paging tree that was touched.
    //
    // VSG's DatabasePager reads from several threads at once
    // (vsg::ResourceHints::numDatabasePagerReadThreads, 4 by default), so this
    // list must be guarded: an unguarded push_back from concurrent readers
    // reallocates while another thread appends, which corrupts the heap.
    struct ReadInfo
    {
        vsg::Path file;
        std::vector<std::string> children;
    };

    std::vector<ReadInfo> readInfos() const
    {
        std::lock_guard<std::mutex> lock(_pagingMutex);
        return _pagingTree;
    }

    bool getFeatures(Features& features) const override
    {
        features.extensionFeatureMap[".osgb"] = static_cast<FeatureMask>(READ_FILENAME);
        features.extensionFeatureMap[".osgt"] = static_cast<FeatureMask>(READ_FILENAME);
        return true;
    }

    vsg::ref_ptr<vsg::Object> read(const vsg::Path& filename,
                                   vsg::ref_ptr<const vsg::Options> options) const override
    {
        const vsg::Path ext = vsg::lowerCaseFileExtension(filename);
        if (ext != ".osgb" && ext != ".osgt") return vsg::ref_ptr<vsg::Object>();

        const vsg::Path path = vsg::findFile(filename, options);
        if (path.empty()) return vsg::ref_ptr<vsg::Object>();

        // Children referenced by this file are named relative to its own
        // directory, so hand the pager (and any nested read) a path list that
        // includes it.
        vsg::ref_ptr<vsg::Options> childOptions = vsg::Options::create(*options);
        childOptions->paths.push_back(directoryOf(path));

        osgVerse::SceneIR ir;
        if (!osgVerse::readSceneIR(path.string(), ir)) return vsg::ref_ptr<vsg::Object>();

        vsg_ir::VsgSceneBuilder builder(childOptions, pagedLodRatio);
        osgVerse::buildScene(ir, builder);

        // Record the PagedLOD/ProxyNode child file names this file references.
        const std::map<int, std::vector<std::string> > files = builder.pagedChildFiles();
        if (!files.empty())
        {
            ReadInfo info;
            info.file = path;
            for (std::map<int, std::vector<std::string> >::const_iterator it = files.begin();
                 it != files.end(); ++it)
                info.children.insert(info.children.end(), it->second.begin(), it->second.end());

            std::lock_guard<std::mutex> lock(_pagingMutex);
            _pagingTree.push_back(info);
        }

        return builder.root();
    }

protected:
    mutable std::mutex _pagingMutex;
    mutable std::vector<ReadInfo> _pagingTree;
};

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: vsg_plugin_integration <root.osgb|root.osgt> [--list] "
                     "[--load-levels N] [--lod-ratio R] [--max-paged-lod N] "
                     "[--screenshot <image>] [-f <frames>]" << std::endl;
        return 1;
    }

    const vsg::Path filename = argv[1];
    bool listOnly = false;
    std::string screenshotFile;
    double lodRatio = 0.25;
    int loadLevels = 0, maxPagedLod = 0, numFrames = -1;
    for (int i = 2; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--list") listOnly = true;
        else if (a == "--screenshot" && i + 1 < argc) screenshotFile = argv[++i];
        else if (a == "--lod-ratio" && i + 1 < argc) lodRatio = std::stod(argv[++i]);
        else if (a == "--load-levels" && i + 1 < argc) loadLevels = std::stoi(argv[++i]);
        else if (a == "--max-paged-lod" && i + 1 < argc) maxPagedLod = std::stoi(argv[++i]);
        else if ((a == "-f" || a == "--frames") && i + 1 < argc) numFrames = std::stoi(argv[++i]);
    }

    // -----------------------------------------------------------------------
    // Register the plugin.
    // -----------------------------------------------------------------------
    vsg::ref_ptr<vsg::Options> options = vsg::Options::create();
    options->add(vsgXchange::all::create());       // jpg/png/... for the embedded textures

    vsg::ref_ptr<OsgbReaderWriter> osgbReader = OsgbReaderWriter::create();
    osgbReader->pagedLodRatio = lodRatio;
    options->add(osgbReader);                      // .osgb/.osgt through the generic reader
    options->paths.push_back(directoryOf(filename));

    // The root file itself goes through the very same plugin.
    vsg::ref_ptr<vsg::Node> root = vsg::read_cast<vsg::Node>(filename, options);
    if (!root)
    {
        std::cerr << "Failed to read " << filename << std::endl;
        return 1;
    }

    std::cout << "Root        : " << root->className() << std::endl;

    if (loadLevels > 0)
    {
        int loaded = 0;
        std::cout << "Preloading " << loadLevels << " level(s):" << std::endl;
        preloadPagedLOD(root.get(), 1, loadLevels, loaded);
        std::cout << "Preloaded   : " << loaded << " external files" << std::endl;
    }

    // Count the PagedLODs now in the graph (the root plus whatever was loaded).
    PagedLODCounter counter;
    root->accept(counter);
    std::cout << "PagedLODs   : " << counter.count << std::endl;

    vsg::ComputeBounds computeBounds;
    root->accept(computeBounds);
    vsg::dbox bounds = computeBounds.bounds;
    if (!bounds.valid()) bounds = vsg::dbox(vsg::dvec3(-1.0, -1.0, -1.0), vsg::dvec3(1.0, 1.0, 1.0));
    std::cout << "Bounds      : " << bounds.min << " - " << bounds.max << std::endl;

    if (listOnly)
    {
        // The child FileName list each paged node exposed, and what was read.
        std::cout << "Paging tree (file -> child FileName list):" << std::endl;
        const std::vector<OsgbReaderWriter::ReadInfo> infos = osgbReader->readInfos();
        for (size_t i = 0; i < infos.size(); ++i)
        {
            std::cout << "  " << infos[i].file << " ->";
            for (size_t k = 0; k < infos[i].children.size(); ++k)
                std::cout << " " << infos[i].children[k];
            std::cout << std::endl;
        }
        return 0;
    }

    // -----------------------------------------------------------------------
    // Viewer
    // -----------------------------------------------------------------------
    vsg::ref_ptr<vsg::WindowTraits> traits = vsg::WindowTraits::create();
    traits->windowTitle = "generic_osgwrappers (paging) - " + filename.string();
    traits->debugLayer = false;

    vsg::ref_ptr<vsg::Viewer> viewer = vsg::Viewer::create();
    vsg::ref_ptr<vsg::Window> window = vsg::Window::create(traits);
    if (!window)
    {
        std::cerr << "Failed to create a Vulkan window (need a display / GPU)." << std::endl;
        return 3;
    }
    viewer->addWindow(window);

    vsg::dvec3 center = (bounds.min + bounds.max) * 0.5;
    double radius = vsg::length(bounds.max - bounds.min) * 0.5;
    if (radius <= 0.0) radius = 1.0;

    vsg::ref_ptr<vsg::LookAt> lookAt = vsg::LookAt::create(
        center + vsg::dvec3(0.0, -radius * 3.5, radius * 1.5), center, vsg::dvec3(0.0, 0.0, 1.0));
    vsg::ref_ptr<vsg::Perspective> perspective = vsg::Perspective::create(
        30.0, (double)window->extent2D().width / (double)window->extent2D().height,
        radius * 0.001, radius * 20.0);
    vsg::ref_ptr<vsg::Camera> camera = vsg::Camera::create(
        perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // Pre-size the Vulkan resources for the paged database.
    {
        vsg::CollectResourceRequirements collect;
        root->accept(collect);
        root->setObject("ResourceHints", collect.createResourceHints(
            loadLevels > 0 ? (uint32_t)(loadLevels + 1) : 4u));
    }

    vsg::ref_ptr<vsg::CommandGraph> commandGraph = vsg::createCommandGraphForView(window, camera, root);
    viewer->assignRecordAndSubmitTaskAndPresentation({ commandGraph });

    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));

    viewer->compile();

    if (maxPagedLod > 0)
    {
        for (auto& task : viewer->recordAndSubmitTasks)
            if (task->databasePager)
                task->databasePager->targetMaxNumPagedLODWithHighResSubgraphs = maxPagedLod;
    }

    uint32_t frameCount = 0;
    while (viewer->advanceToNextFrame() && (numFrames < 0 || (numFrames--) > 0))
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();

        ++frameCount;
        if (!screenshotFile.empty() && frameCount >= 30)
        {
            std::cout << (vsg_ir::saveScreenshot(window, screenshotFile, options) ?
                          "Screenshot  : " : "Failed to write screenshot: ") << screenshotFile << std::endl;
            break;
        }
    }
    return 0;
}
