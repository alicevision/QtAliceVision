# Target srcs
set(PLUGIN_SOURCES
    CurveAxisTicks.cpp
    CurveBandModel.cpp
    CurveModel.cpp
    CurveRenderer.cpp
    CurveViewer.cpp
    )

set(PLUGIN_HEADERS
    plugin.hpp
    CurveAxisTicks.hpp
    CurveBandModel.hpp
    CurveModel.hpp
    CurveRenderer.hpp
    CurveViewer.hpp
    )


# Qt module dependency
find_package(Qt6 COMPONENTS Core Gui Qml Quick REQUIRED)
find_package(Qt6 COMPONENTS ShaderTools REQUIRED)


# Target properties
add_library(curveViewerPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

if(MSVC)
    target_compile_options(curveViewerPlugin PUBLIC /W4)
else()
    target_compile_options(curveViewerPlugin PUBLIC -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic)
endif()

qt6_add_shaders(curveViewerPlugin "curveViewerPlugin_shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX "/curveViewerShaders"
    BASE shaders
    FILES shaders/CurveLine.vert shaders/CurveLine.frag shaders/CurveFill.vert shaders/CurveFill.frag
    )

target_link_libraries(curveViewerPlugin
  PUBLIC
    aliceVision_sfmData
    aliceVision_sfmDataIO
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
  PRIVATE
    # QRhi (rhi/qrhi.h), used by CurveRenderer
    Qt6::GuiPrivate
)

set_target_properties(curveViewerPlugin
        PROPERTIES
        DEBUG_POSTFIX ""
        FOLDER "curveViewerPlugin"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        )


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/CurveViewer)
install(TARGETS curveViewerPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/CurveViewer")
