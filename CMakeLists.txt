cmake_minimum_required(VERSION 3.30)
project(QtAliceVisionPlugin LANGUAGES CXX VERSION 0.2.0)

# Build options
option(BUILD_IMAGEIO "Build qtAliceVisionImageIO plugin" ON)
option(BUILD_DEPTHMAPENTITY "Build depthMapEntity plugin" ON)
option(BUILD_SFM "Build qtAliceVision and qmlSfmData plugin" ON)
option(BUILD_CURVEVIEWER "Build curveViewer plugin" ON)
option(BUILD_MESHVIEWER "Build meshViewer plugin" ON)

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
    Concurrent
    Charts
    Qml
)

# Since Qt 6.9, private module targets (Qt6::GuiPrivate, needed for RHI) are
# shipped as separate packages that must be requested explicitly.
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.9)
    find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()

qt_standard_project_setup()

add_compile_definitions(
    # for performance reasons let Qt handle jpeg, png and ico formats
    QT_ALICEVISIONIMAGEIO_USE_FORMATS_BLACKLIST
    # comment to get qDebug outputs
    QT_NO_DEBUG_OUTPUT
)

# AliceVision dependency
find_package(AliceVision REQUIRED)

# Builds
add_subdirectory(src)
