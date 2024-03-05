# Target srcs
set(PLUGIN_SOURCES
    FeaturesViewer.cpp
    MFeatures.cpp
    MSfMData.cpp
    MViewStats.cpp
    MTracks.cpp
    FloatImageViewer.cpp
    FloatTexture.cpp
    Surface.cpp
    MSfMDataStats.cpp
    PanoramaViewer.cpp
    Painter.cpp
    SequenceCache.cpp
    SingleImageLoader.cpp
    )

set(PLUGIN_HEADERS
    plugin.hpp
    FeaturesViewer.hpp
    MFeatures.hpp
    MSfMData.hpp
    MTracks.hpp
    MViewStats.hpp
    FloatImageViewer.hpp
    FloatTexture.hpp
    MSfMDataStats.hpp
    PanoramaViewer.hpp
    Surface.hpp
    Painter.hpp
    ImageServer.hpp
    SequenceCache.hpp
    SingleImageLoader.hpp
    )

set(PLUGIN_MOCS
    FeaturesViewer.hpp
    MFeatures.hpp
    MSfMData.hpp
    MViewStats.hpp
    MTracks.hpp
    MSfMDataStats.hpp
    SequenceCache.hpp
    SingleImageLoader.hpp
    )


# Qt module dependency
find_package(Qt6 COMPONENTS Quick REQUIRED)
find_package(Qt6 COMPONENTS Qml REQUIRED)
find_package(Qt6 COMPONENTS Charts REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Gui)
find_package(Qt6 COMPONENTS ShaderTools)


# Target properties
add_library(qtAliceVisionPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS} ${PLUGIN_MOCS})


if(MSVC)
    target_compile_options(qtAliceVisionPlugin PUBLIC /W4)
else()
    target_compile_options(qtAliceVisionPlugin PUBLIC -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic)
endif()

find_package(Qt6 COMPONENTS ShaderTools)
qt6_add_shaders(qtAliceVisionPlugin "qtAliceVisionPlugin_shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX "/shaders"
    FILES FloatImageViewer.vert FloatImageViewer.frag
    )

target_link_libraries(qtAliceVisionPlugin
  PUBLIC
    aliceVision_feature
    aliceVision_sfmDataIO
    aliceVision_sfm
    aliceVision_system
    aliceVision_image
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::Charts
    Qt6::GuiPrivate
)

set_target_properties(qtAliceVisionPlugin
        PROPERTIES
        DEBUG_POSTFIX ""
        FOLDER "qtAliceVisionPlugin"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        )


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/AliceVision)
install(TARGETS qtAliceVisionPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/AliceVision")
