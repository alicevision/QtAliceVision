# Target srcs
set(PLUGIN_SOURCES
    AlembicEntity.cpp
    BaseAlembicObject.cpp
    CameraLocatorEntity.cpp
    IOThread.cpp
    PointCloudEntity.cpp
    ObservationsEntity.cpp
    )

set(PLUGIN_HEADERS
    AlembicEntity.hpp
    BaseAlembicObject.hpp
    CameraLocatorEntity.hpp
    IOThread.hpp
    PointCloudEntity.hpp
    ObservationsEntity.hpp
    plugin.hpp
    )


# Qt module dependency
find_package(Qt5 COMPONENTS Qml REQUIRED)
find_package(Qt5 COMPONENTS 3DCore REQUIRED)
find_package(Qt5 COMPONENTS 3DRender REQUIRED)
find_package(Qt5 COMPONENTS 3DExtras REQUIRED)


# Alembic dependency
find_package(Alembic 1.7 REQUIRED)


# Target properties
add_library(alembicEntityQmlPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

if(MSVC)
    target_compile_options(alembicEntityQmlPlugin PUBLIC /W4)
else()
    target_compile_options(alembicEntityQmlPlugin PUBLIC -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic)
endif()

target_include_directories(alembicEntityQmlPlugin
    PUBLIC
    ${ILMBASE_INCLUDE_DIR}
    )

target_link_libraries(alembicEntityQmlPlugin
    PUBLIC
    Qt5::Core
    Qt5::Qml
    Qt5::3DCore
    Qt5::3DRender
    Alembic::Alembic
    Qt5::3DExtras
    aliceVision_sfmData
    aliceVision_sfmDataIO
    )

set_target_properties(alembicEntityQmlPlugin
        PROPERTIES
        DEBUG_POSTFIX "d"
        FOLDER "alembicEntityQmlPlugin"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        )


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/AlembicEntity)
install(TARGETS alembicEntityQmlPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/AlembicEntity")
