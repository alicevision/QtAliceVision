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
    ImageCache.cpp
    AsyncFetcher.cpp
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
    ShaderImageViewer.hpp
    Painter.hpp
    ImageServer.hpp
    SequenceCache.hpp
    SingleImageLoader.hpp
    ImageCache.hpp
    AsyncFetcher.hpp
    
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
    AsyncFetcher.hpp
    )


# Qt module dependency
find_package(Qt5 COMPONENTS Quick REQUIRED)
find_package(Qt5 COMPONENTS Qml REQUIRED)
find_package(Qt5 COMPONENTS Charts REQUIRED)


# Target properties
add_library(qtAliceVisionPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS} ${PLUGIN_MOCS})


if(MSVC)
    target_compile_options(qtAliceVisionPlugin PUBLIC /W4)
else()
    target_compile_options(qtAliceVisionPlugin PUBLIC -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic)
endif()

target_link_libraries(qtAliceVisionPlugin
  PUBLIC
    aliceVision_feature
    aliceVision_sfmDataIO
    aliceVision_sfm
    aliceVision_system
    aliceVision_image
    Qt5::Core
    Qt5::Qml
    Qt5::Quick
    Qt5::Charts
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
