# Target srcs
set(PLUGIN_SOURCES
    FeaturesViewer.cpp
    MFeature.cpp
    MFeatures.cpp
    MSfMData.cpp
    MViewStats.cpp
    MTracks.cpp
    FloatImageViewer.cpp
    FloatTexture.cpp
    Surface.cpp
    MSfMDataStats.cpp
    PanoramaViewer.cpp
    )

set(PLUGIN_HEADERS
    plugin.hpp
    FeaturesViewer.hpp
    MFeature.hpp
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
    )

set(PLUGIN_MOCS
    FeaturesViewer.hpp
    MFeature.hpp
    MFeatures.hpp
    MSfMData.hpp
    MViewStats.hpp
    MTracks.hpp
    MSfMDataStats.hpp
    )


# Qt module dependency
find_package(Qt5 COMPONENTS Quick REQUIRED)
find_package(Qt5 COMPONENTS Qml REQUIRED)
find_package(Qt5 COMPONENTS Charts REQUIRED)


# Target properties
add_library(qtAliceVisionPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS} ${PLUGIN_MOCS})

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
        DEBUG_POSTFIX "d"
        FOLDER "qtAliceVisionPlugin"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        )


# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/AliceVision)
install(TARGETS qtAliceVisionPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/AliceVision")
