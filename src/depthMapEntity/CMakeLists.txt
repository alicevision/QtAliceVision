# Target srcs
set(PLUGIN_SOURCES
    DepthMapEntity.cpp
    )

set(PLUGIN_HEADERS
    DepthMapEntity.hpp
    mv_matrix3x3.hpp
    mv_point2d.hpp
    mv_point3d.hpp
    plugin.hpp
    )

# Qt module dependencies
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Qml REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DCore REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DRender REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS 3DExtras REQUIRED)

# OpenImageIO dependency
find_package(OpenImageIO REQUIRED)
if(OPENIMAGEIO_FOUND)
    message(STATUS "OpenImageIO found.")
else()
    message(SEND_ERROR "Failed to find OpenImageIO.")
endif()

# Target properties
add_library(depthMapEntityQmlPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

target_include_directories(depthMapEntityQmlPlugin
  PUBLIC
    ${OPENIMAGEIO_INCLUDE_DIRS}
    ${OPENEXR_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}
)
target_link_libraries(depthMapEntityQmlPlugin
  PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
)

target_link_libraries(depthMapEntityQmlPlugin
  PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::3DCore
    Qt${QT_VERSION_MAJOR}::3DRender
    Qt${QT_VERSION_MAJOR}::3DExtras
)

# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity)
install(TARGETS depthMapEntityQmlPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity")
