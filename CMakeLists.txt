# generic_wrappers: a zero-osg-dependency .osgt/.osgb serialization layer
cmake_minimum_required(VERSION 3.10)

project(osgVerseGenericWrappers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# The dependency-free core library. Only the standard library is used; the
# generic_osg/*.cpp sources statically register rewrappers through
# REGISTER_OBJECT_WRAPPER.
# ---------------------------------------------------------------------------
file(GLOB GENERIC_OSG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/generic_osg/*.cpp")

# The remaining osg libraries, ported one directory per library.
set(GENERIC_EXTRA_LIBS osgterrain osganimation osgparticle osgvolume osgsim
    osgmanipulator osgshadow osgui osgga osgfx osgviewer osgtext osgutil)
set(GENERIC_EXTRA_SOURCES "")
foreach(_lib ${GENERIC_EXTRA_LIBS})
    file(GLOB _sources "${CMAKE_CURRENT_SOURCE_DIR}/generic_${_lib}/*.cpp")
    list(APPEND GENERIC_EXTRA_SOURCES ${_sources})
endforeach()

add_library(osgVerseGenericReserializer STATIC
    GenericReserializer.cpp
    GenericInputStream.cpp
    GenericReader.cpp
    SceneIR.cpp
    SceneIRRecorder.cpp
    ${GENERIC_OSG_SOURCES}
    ${GENERIC_EXTRA_SOURCES}
)
target_include_directories(osgVerseGenericReserializer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# The static registrars are never referenced explicitly, so MSVC/GCC would drop
# the object files unless the archive is force-included in full.
#
# ENABLE_STATIC_LIBRARY=ON switches to a plain static link instead: nothing is
# force-included, and each executable keeps the registrars alive by including
# GenericStaticWrappers.h (gated on the OSGVERSE_GENERIC_USE_STATIC_ACTIVATION
# define set below) -- the equivalent of osgDB's USE_SERIALIZER_WRAPPER_LIBRARY.
option(ENABLE_STATIC_LIBRARY
    "Link the core as a plain static archive and activate wrappers explicitly" OFF)

function(link_generic_library target)
    if(ENABLE_STATIC_LIBRARY)
        target_link_libraries(${target} PRIVATE osgVerseGenericReserializer)
        target_compile_definitions(${target} PRIVATE OSGVERSE_GENERIC_USE_STATIC_ACTIVATION=1)
    elseif(MSVC)
        target_link_libraries(${target} PRIVATE osgVerseGenericReserializer)
        target_link_options(${target} PRIVATE "/WHOLEARCHIVE:osgVerseGenericReserializer")
    else()
        target_link_libraries(${target} PRIVATE
            "-Wl,--whole-archive" osgVerseGenericReserializer "-Wl,--no-whole-archive")
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Dependency-free executables
# ---------------------------------------------------------------------------
add_executable(reserializing_test reserializing_test.cpp)
link_generic_library(reserializing_test)

add_executable(null_integration examples/null/null_integration.cpp)
link_generic_library(null_integration)

# ---------------------------------------------------------------------------
# Engine-specific examples: one sub-directory per engine under examples/.
#   examples/null/      zero-dependency IR dump
#   examples/vsg/       VulkanSceneGraph (-DENABLE_VSG=ON -DVSG_ROOT=<sdk>)
#   examples/threejs/   three.js / WebGL (built under Emscripten, see below)
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# VulkanSceneGraph integration (uses the installed vsg / vsgXchange packages).
# ---------------------------------------------------------------------------
option(ENABLE_VSG "Build the VulkanSceneGraph integration example" OFF)
set(VSG_ROOT "" CACHE PATH "VulkanSceneGraph SDK root (include/ + lib/cmake/)")

if(ENABLE_VSG)
    if(NOT VSG_ROOT)
        message(FATAL_ERROR "VSG_ROOT is required when ENABLE_VSG=ON")
    endif()

    list(APPEND CMAKE_PREFIX_PATH
        ${VSG_ROOT}/lib/cmake/vsg
        ${VSG_ROOT}/lib/cmake/vsgXchange)
    find_package(vsg REQUIRED)
    find_package(vsgXchange REQUIRED)

    add_executable(vsg_integration examples/vsg/vsg_integration.cpp)
    link_generic_library(vsg_integration)
    target_link_libraries(vsg_integration PRIVATE vsg::vsg vsgXchange::vsgXchange)

    # Same adapter, but registered as a vsg::ReaderWriter so VSG's DatabasePager
    # can dynamically page osgb/osgt PagedLOD / ProxyNode children.
    add_executable(vsg_plugin_integration examples/vsg/vsg_plugin_integration.cpp)
    link_generic_library(vsg_plugin_integration)
    target_link_libraries(vsg_plugin_integration PRIVATE vsg::vsg vsgXchange::vsgXchange)
endif()

# ---------------------------------------------------------------------------
# three.js / WebGL example. Built only under Emscripten, which exposes the
# reader to JavaScript through embind; the .js/.wasm land in
# examples/threejs/dist/ next to the HTML/JS that consume them:
#
#   emcmake cmake -S . -B build-wasm -DENABLE_STATIC_LIBRARY=ON
#   cmake --build build-wasm
#   python3 examples/threejs/serve.py
# ---------------------------------------------------------------------------
if(EMSCRIPTEN)
    add_executable(generic_osg_wasm examples/threejs/wrappers_wasm.cpp)
    link_generic_library(generic_osg_wasm)
    set_target_properties(generic_osg_wasm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/threejs/dist)
    target_link_options(generic_osg_wasm PRIVATE
        --bind
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sEXPORT_NAME=createGenericOSG"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sENVIRONMENT=web,node")
endif()
