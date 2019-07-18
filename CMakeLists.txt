cmake_minimum_required(VERSION 3.3)
project(QtAliceVisionPlugin LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Qml REQUIRED)

# AliceVision dependency
find_package(AliceVision REQUIRED)

add_subdirectory(src)
