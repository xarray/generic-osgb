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
function(link_generic_library target)
    if(MSVC)
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

add_executable(null_integration examples/null_integration.cpp)
link_generic_library(null_integration)

enable_testing()
add_test(NAME reserializing_test
         COMMAND reserializing_test ${CMAKE_CURRENT_SOURCE_DIR}/../examples/osgemscripten/box.osgt)

# ---------------------------------------------------------------------------
# Engine-specific examples (each may pull in its own engine SDK). Each engine is
# enabled with -DENABLE_<ENGINE>=ON plus its -D<ENGINE>_ROOT=<sdk>.
#   cmake -S . -B build -DENABLE_OSG=ON -DOSG_ROOT=/path/to/osg-sdk
#   cmake -S . -B build -DENABLE_VSG=ON -DVSG_ROOT=/path/to/vsg-sdk
# ---------------------------------------------------------------------------
option(ENABLE_OSG "Build the OpenSceneGraph integration example" OFF)
set(OSG_ROOT "" CACHE PATH "OpenSceneGraph SDK root (include/ + lib/)")

if(ENABLE_OSG)
    if(NOT OSG_ROOT)
        message(FATAL_ERROR "OSG_ROOT is required when ENABLE_OSG=ON")
    endif()

    add_executable(osg_integration examples/osg_integration.cpp)
    target_include_directories(osg_integration PRIVATE ${OSG_ROOT}/include)
    target_link_directories(osg_integration PRIVATE ${OSG_ROOT}/lib)
    link_generic_library(osg_integration)
    target_link_libraries(osg_integration PRIVATE
        osgViewer osgGA osgDB osgUtil osg OpenThreads)
endif()

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

    add_executable(vsg_integration examples/vsg_integration.cpp)
    link_generic_library(vsg_integration)
    target_link_libraries(vsg_integration PRIVATE vsg::vsg vsgXchange::vsgXchange)

    # Same adapter, but registered as a vsg::ReaderWriter so VSG's DatabasePager
    # can dynamically page osgb/osgt PagedLOD / ProxyNode children.
    add_executable(vsg_plugin_integration examples/vsg_plugin_integration.cpp)
    link_generic_library(vsg_plugin_integration)
    target_link_libraries(vsg_plugin_integration PRIVATE vsg::vsg vsgXchange::vsgXchange)
endif()
