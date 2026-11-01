message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
cmake_minimum_required(VERSION 3.11)
project(QtAliceVisionPlugin LANGUAGES CXX VERSION 0.1.0)


# Build options
option(BUILD_IMAGEIO "Build qtAliceVisionImageIO plugin" ON)
option(BUILD_DEPTHMAPENTITY "Build depthMapEntity plugin" ON)
option(BUILD_SFM "Build qtAliceVision and qmlSfmData plugin" ON)
option(BUILD_TESTS "Build the QML tests (run against the installed plugins)" OFF)
option(BUILD_CURVEVIEWER "Build curveViewer plugin" ON)

message(STATUS "BUILD_IMAGEIO: ${BUILD_IMAGEIO}")
message(STATUS "BUILD_DEPTHMAPENTITY: ${BUILD_DEPTHMAPENTITY}")
message(STATUS "BUILD_SFM: ${BUILD_SFM}")
message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "BUILD_CURVEVIEWER: ${BUILD_CURVEVIEWER}")


# CMake Find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Qt
find_package(Qt6Core REQUIRED)
find_package(Qt6Quick REQUIRED)
find_package(Qt6Qml REQUIRED)
find_package(Qt6Charts REQUIRED)
find_package(Qt6Gui REQUIRED)
# Since Qt 6.10, private modules must be requested explicitly (before, Qt6Gui defines Qt6::GuiPrivate).
if(Qt6Gui_VERSION VERSION_GREATER_EQUAL 6.10)
    find_package(Qt6GuiPrivate REQUIRED)
endif()
find_package(Qt6 COMPONENTS ShaderTools)


set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)
add_definitions(-DQT_NO_KEYWORDS)

# comment to get qDebug outputs
add_definitions(-DQT_NO_DEBUG_OUTPUT)

# for performance reasons let Qt handle jpeg, png and ico formats
add_definitions(-DQT_ALICEVISIONIMAGEIO_USE_FORMATS_BLACKLIST)


# AliceVision dependency
find_package(AliceVision REQUIRED)


# Builds
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
