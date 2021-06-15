# Target srcs
set(PLUGIN_SOURCES
    AlembicEntity.cpp
    BaseAlembicObject.cpp
    CameraLocatorEntity.cpp
    IOThread.cpp
    PointCloudEntity.cpp
    )

set(PLUGIN_HEADERS
    AlembicEntity.hpp
    BaseAlembicObject.hpp
    CameraLocatorEntity.hpp
    IOThread.hpp
    PointCloudEntity.hpp
    plugin.hpp
    )

# Qt module dependencies
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Qml REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DCore REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DExtras REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DRender REQUIRED)

# Alembic dependency
find_package(Alembic 1.7 REQUIRED)

# Add missing include path to IlmBase/OpenEXR include folder
find_package(IlmBase REQUIRED)

# Target properties
add_library(alembicEntityQmlPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

target_link_libraries(alembicEntityQmlPlugin
  PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::3DCore
    Qt${QT_VERSION_MAJOR}::3DRender
    Alembic::Alembic
  PRIVATE
    Qt${QT_VERSION_MAJOR}::3DExtras
)

set_target_properties(alembicEntityQmlPlugin
  PROPERTIES
  DEBUG_POSTFIX "d"
  FOLDER "alembicEntityQmlPlugin"
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

target_include_directories(alembicEntityQmlPlugin PUBLIC ${ILMBASE_INCLUDE_DIR})

# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/AlembicEntity)
install(TARGETS alembicEntityQmlPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/AlembicEntity")
