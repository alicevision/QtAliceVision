cmake_minimum_required(VERSION 3.3)
project(QtAliceVisionPlugin)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Quick REQUIRED)
find_package(Qt5Qml REQUIRED)

# Alembic dependency
find_package(AliceVision REQUIRED)

add_subdirectory(src)
