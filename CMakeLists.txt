cmake_minimum_required(VERSION 3.11)
project(QtAliceVisionPlugin LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt5Charts REQUIRED)
set(CMAKE_AUTOMOC ON)

# AliceVision dependency
find_package(IlmBase REQUIRED)
find_package(AliceVision REQUIRED)

add_subdirectory(src)
