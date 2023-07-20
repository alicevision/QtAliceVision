cmake_minimum_required(VERSION 3.3)
project(alembicEntityQmlPlugin LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt dependency
if(POLICY CMP0043)
    cmake_policy(SET CMP0043 OLD)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5Core REQUIRED)
find_package(Qt5Qml REQUIRED)
find_package(Qt53DCore REQUIRED)
find_package(Qt53DExtras REQUIRED)
find_package(Qt53DRender REQUIRED)
add_definitions(-DQT_NO_KEYWORDS)

# Alembic dependency
find_package(Alembic 1.7 REQUIRED)

add_subdirectory(src)
