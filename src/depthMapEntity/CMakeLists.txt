# Target srcs
set(PLUGIN_SOURCES
    DepthMapEntity.cpp
    )

set(PLUGIN_HEADERS
    plugin.hpp
    DepthMapEntity.hpp
    )


# Qt module dependency
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS Qml REQUIRED)
find_package(Qt5 COMPONENTS Quick REQUIRED)
find_package(Qt5 COMPONENTS 3DCore REQUIRED)
find_package(Qt5 COMPONENTS 3DRender REQUIRED)
find_package(Qt5 COMPONENTS 3DExtras REQUIRED)


# OpenImageIO dependency
find_package(OpenImageIO REQUIRED)


# Target properties
add_library(depthMapEntityPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

target_include_directories(depthMapEntityPlugin 
    PUBLIC 
    ${OPENIMAGEIO_INCLUDE_DIRS}
    )

target_link_libraries(depthMapEntityPlugin
      PUBLIC
      ${OPENIMAGEIO_LIBRARIES}
      aliceVision_image
      Qt5::Core
      Qt5::Gui
      Qt5::Qml
      Qt5::Quick
      Qt5::3DCore
      Qt5::3DRender
      Qt5::3DExtras
      )

set_target_properties(depthMapEntityPlugin
      PROPERTIES
      DEBUG_POSTFIX "d"
      FOLDER "depthMapEntityPlugin"
      $SOVERSION ${PROJECT_VERSION_MAJOR}
      VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
      )


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity)
install(TARGETS depthMapEntityPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity")
