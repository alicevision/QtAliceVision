message("CMake version: ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}")
cmake_minimum_required(VERSION 3.11)
project(QtAliceVisionPlugin LANGUAGES CXX VERSION 0.1.0)


# Build options
option(BUILD_IMAGEIO "Build qtAliceVisionImageIO plugin" ON)
option(BUILD_DEPTHMAPENTITY "Build depthMapEntity plugin" ON)
option(BUILD_SFM "Build qtAliceVision plugin" ON)

message(STATUS "BUILD_IMAGEIO: ${BUILD_IMAGEIO}")
message(STATUS "BUILD_DEPTHMAPENTITY: ${BUILD_DEPTHMAPENTITY}")
message(STATUS "BUILD_SFM: ${BUILD_SFM}")


# CMake Find modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)


# C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6Core REQUIRED)
find_package(Qt6Quick REQUIRED)
find_package(Qt6Qml REQUIRED)
find_package(Qt6Charts REQUIRED)
find_package(Qt6 COMPONENTS ShaderTools)
find_package(IMath REQUIRED)
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
if(BUILD_IMAGEIO)
	add_subdirectory(src/qtAliceVisionImageIO)
endif()

if(BUILD_DEPTHMAPENTITY)
	add_subdirectory(src/depthMapEntity)
endif()

if(BUILD_SFM)
	add_subdirectory(src)
endif()
