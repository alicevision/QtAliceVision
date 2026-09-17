cmake_minimum_required(VERSION 4.0)
project(QtAliceVisionPlugin LANGUAGES CXX VERSION 0.2.0)

# Build options
option(BUILD_IMAGEIO "Build qtAliceVisionImageIO plugin" ON)
option(BUILD_DEPTHMAPENTITY "Build depthMapEntity plugin" ON)
option(BUILD_SFM "Build qtAliceVision and qmlSfmData plugin" ON)

# C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# For ide
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    OpenGL
    ShaderTools
    Charts
    Qml
)

qt_standard_project_setup()

# for performance reasons let Qt handle jpeg, png and ico formats
add_definitions(-DQT_ALICEVISIONIMAGEIO_USE_FORMATS_BLACKLIST)

# AliceVision dependency
find_package(AliceVision REQUIRED)

# Builds
add_subdirectory(src)
