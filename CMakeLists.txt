# generic_wrappers：零 osg 依赖的 .osgt/.osgb 序列化层
cmake_minimum_required(VERSION 3.10)

project(osgVerseGenericWrappers)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

file(GLOB GENERIC_OSG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/generic_osg/*.cpp")

add_library(osgVerseGenericReserializer STATIC
    GenericReserializer.cpp
    GenericInputStream.cpp
    GenericReader.cpp
    ${GENERIC_OSG_SOURCES}
)
target_include_directories(osgVerseGenericReserializer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(reserializing_test
    reserializing_test.cpp
)

# generic_osg/*.cpp 通过 REGISTER_OBJECT_WRAPPER 静态注册 rewrapper，
# MSVC/GCC 默认会丢弃未被引用的静态库目标文件，需强制全量包含。
if(MSVC)
    target_link_libraries(reserializing_test PRIVATE osgVerseGenericReserializer)
    target_link_options(reserializing_test PRIVATE "/WHOLEARCHIVE:osgVerseGenericReserializer")
else()
    target_link_libraries(reserializing_test PRIVATE
        "-Wl,--whole-archive" osgVerseGenericReserializer "-Wl,--no-whole-archive")
endif()

enable_testing()
add_test(NAME reserializing_test
         COMMAND reserializing_test ${CMAKE_CURRENT_SOURCE_DIR}/../examples/osgemscripten/box.osgt)
