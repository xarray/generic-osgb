// vsg_integration: read an .osgt/.osgb file and render it with VulkanSceneGraph.
//
// The reader / neutral IR / VSG adapter all live in vsg_scene_builder.h, which is
// shared with vsg_plugin_integration.cpp (that example registers the very same
// adapter as a vsg::ReaderWriter so VSG's DatabasePager can pull in osgb/osgt
// PagedLOD children through the generic reader).
//
// Build: -DENABLE_VSG=ON -DVSG_ROOT=<sdk>
// Usage: vsg_integration <file.osgt|file.osgb> [--list] [--screenshot <image>]

#include <vsg/all.h>
#include <vsgXchange/all.h>

#include <iostream>
#include <map>
#include <string>

#include "SceneIR.h"
#include "vsg_scene_builder.h"

#ifdef OSGVERSE_GENERIC_USE_STATIC_ACTIVATION
#include "GenericStaticWrappers.h"   // keep every wrapper registrar in a plain static link
#endif

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: vsg_integration <file.osgt|file.osgb> [--list] [--screenshot <image>]" << std::endl;
        return 1;
    }
    const std::string filename = argv[1];
    bool listOnly = false;
    std::string screenshotFile;
    for (int i = 2; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--list") listOnly = true;
        else if (a == "--screenshot" && i + 1 < argc) screenshotFile = argv[++i];
    }

    // Let relative texture/image names in the asset resolve.
    std::string dir = filename;
    std::string::size_type s = dir.find_last_of("/\\");
    dir = (s == std::string::npos) ? "." : dir.substr(0, s);

    vsg::ref_ptr<vsg::Options> options = vsg::Options::create();
    options->add(vsgXchange::all::create());
    options->paths.push_back(dir);

    osgVerse::SceneIR ir;
    if (!osgVerse::readSceneIR(filename, ir))
    {
        std::cerr << "Failed to read " << filename << std::endl;
        return 1;
    }

    vsg_ir::VsgSceneBuilder builder(options);
    osgVerse::buildScene(ir, builder);

    std::cout << "IR objects  : " << ir.objects.size() << " (" << ir.events.size() << " events)" << std::endl;
    std::cout << "Rebuilt     : " << builder.created() << " vsg objects" << std::endl;
    std::cout << "Draws       : " << builder.draws() << std::endl;
    if (builder.pagedLods())
        std::cout << "PagedLODs   : " << builder.pagedLods() << std::endl;
    if (builder.failedImages())
        std::cout << "Missing     : " << builder.failedImages() << " images not found on disk" << std::endl;

    const std::map<std::string, unsigned int>& unh = builder.unhandled();
    if (!unh.empty())
    {
        std::cout << "Unhandled properties/commands (" << unh.size() << "):" << std::endl;
        for (std::map<std::string, unsigned int>::const_iterator it = unh.begin(); it != unh.end(); ++it)
            std::cout << "  " << it->first << " x" << it->second << std::endl;
    }

    vsg::ref_ptr<vsg::Node> root = builder.root();
    if (!root) { std::cerr << "No renderable root rebuilt." << std::endl; return 2; }

    std::cout << "Root        : " << root->className() << std::endl;

    // Frame the scene with a LookAt derived from its bounds.
    vsg::ComputeBounds computeBounds;
    root->accept(computeBounds);
    vsg::dbox bounds = computeBounds.bounds;
    if (!bounds.valid()) bounds = vsg::dbox(vsg::dvec3(-1.0, -1.0, -1.0), vsg::dvec3(1.0, 1.0, 1.0));
    std::cout << "Bounds      : " << bounds.min << " - " << bounds.max << std::endl;

    if (listOnly) return 0;

    // -----------------------------------------------------------------------
    // Viewer
    // -----------------------------------------------------------------------
    vsg::ref_ptr<vsg::WindowTraits> traits = vsg::WindowTraits::create();
    traits->windowTitle = "generic_osgwrappers - " + filename;
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
        center + vsg::dvec3(0.0, -radius * 3.0, radius * 1.5), center, vsg::dvec3(0.0, 0.0, 1.0));

    vsg::ref_ptr<vsg::Perspective> perspective = vsg::Perspective::create(
        30.0, (double)window->extent2D().width / (double)window->extent2D().height,
        radius * 0.01, radius * 20.0);

    vsg::ref_ptr<vsg::Camera> camera = vsg::Camera::create(
        perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // createRenderGraphForView() builds the View, adds a headlight (the osg
    // assets carry no lights of their own, so a Phong pipeline would render
    // black without it) and installs a window resize handler.
    vsg::ref_ptr<vsg::RenderGraph> renderGraph = vsg::createRenderGraphForView(window, camera, root);
    vsg::ref_ptr<vsg::CommandGraph> commandGraph = vsg::CommandGraph::create(window);
    commandGraph->addChild(renderGraph);
    viewer->assignRecordAndSubmitTaskAndPresentation({ commandGraph });

    viewer->addEventHandler(vsg::CloseHandler::create(viewer));
    viewer->addEventHandler(vsg::Trackball::create(camera));

    viewer->compile();

    uint32_t frameCount = 0;
    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();

        // Give the pipeline a couple of frames, then capture and exit.
        if (!screenshotFile.empty() && ++frameCount >= 3)
        {
            std::cout << (vsg_ir::saveScreenshot(window, screenshotFile, options) ?
                          "Screenshot  : " : "Failed to write screenshot: ") << screenshotFile << std::endl;
            break;
        }
    }
    return 0;
}
