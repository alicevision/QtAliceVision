message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
cmake_minimum_required(VERSION 3.4)
project(QtAliceVision LANGUAGES CXX C VERSION 0.1.0)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# C++11 for Qt5
if(${QT_VERSION_MAJOR} EQUAL 5)
    set(CMAKE_CXX_STANDARD 11)
endif()

# C++17 for Qt6
if(${QT_VERSION_MAJOR} EQUAL 6)
    set(CMAKE_CXX_STANDARD 17)
endif()

# CMake Find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON) # needed for automoc

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)

if(Qt${QT_VERSION_MAJOR}Core_FOUND)
    message(STATUS "Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR} found.")
endif()

add_definitions(-DQT_PLUGIN)
add_definitions(-DQT_SHARED)
add_definitions(-DQT_NO_DEBUG)
add_definitions(-DQT_NO_KEYWORDS)
add_definitions(-DQT_NO_DEBUG_OUTPUT) # comment to get qDebug outputs

add_subdirectory(src/imageIOHandler)
add_subdirectory(src/qmlAlembic)
add_subdirectory(src)

# TODO: Make it works for Qt6
# Add to Qt5 only for the moment since 3dcore
# is not part of the distribution anymore.
# Source: https://www.kdab.com/qt-3d-changes-in-qt-6/
if(Qt5_FOUND)
    add_subdirectory(src/depthMapEntity)
endif()
# add_subdirectory(src/depthMapEntity)
