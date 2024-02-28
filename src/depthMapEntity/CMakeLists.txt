# Target srcs
set(PLUGIN_SOURCES
    DepthMapEntity.cpp
    )

set(PLUGIN_HEADERS
    plugin.hpp
    DepthMapEntity.hpp
    )


# Qt module dependency
find_package(Qt6 COMPONENTS Gui REQUIRED)
find_package(Qt6 COMPONENTS Qml REQUIRED)
find_package(Qt6 COMPONENTS Quick REQUIRED)
find_package(Qt6 COMPONENTS 3DCore REQUIRED)
find_package(Qt6 COMPONENTS 3DRender REQUIRED)
find_package(Qt6 COMPONENTS 3DExtras REQUIRED)


# OpenImageIO dependency
find_package(OpenImageIO REQUIRED)


# Target properties
add_library(depthMapEntityPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

if(MSVC)
    target_compile_options(depthMapEntityPlugin PUBLIC /W4)
else()
    target_compile_options(depthMapEntityPlugin PUBLIC -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic)
endif()

target_include_directories(depthMapEntityPlugin 
    PUBLIC 
    ${OPENIMAGEIO_INCLUDE_DIRS}
    )

target_link_libraries(depthMapEntityPlugin
      PUBLIC
      ${OPENIMAGEIO_LIBRARIES}
      aliceVision_image
      aliceVision_mvsData
      aliceVision_numeric
      Qt6::Core
      Qt6::Gui
      Qt6::Qml
      Qt6::Quick
      Qt6::3DCore
      Qt6::3DRender
      Qt6::3DExtras
      )

set_target_properties(depthMapEntityPlugin
      PROPERTIES
      DEBUG_POSTFIX ""
      FOLDER "depthMapEntityPlugin"
      $SOVERSION ${PROJECT_VERSION_MAJOR}
      VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
      )


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity)
install(TARGETS depthMapEntityPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/DepthMapEntity")
