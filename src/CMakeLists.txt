# Target srcs
set(PLUGIN_SOURCES
    FeaturesViewer.cpp
    MFeature.cpp
    MSfMData.cpp
    MViewStats.cpp
    MTracks.cpp
    FloatImageViewer.cpp
    FloatTexture.cpp
    MSfMDataStats.cpp
    )

set(PLUGIN_HEADERS
    FeaturesViewer.hpp
    plugin.hpp
    MFeature.hpp
    MSfMData.hpp
    MTracks.hpp
    MViewStats.hpp
    FloatImageViewer.hpp
    FloatTexture.hpp
    MSfMDataStats.hpp
    )

set(PLUGIN_MOCS
    FeaturesViewer.hpp
    MFeature.hpp
    MSfMData.hpp
    MViewStats.hpp
    MTracks.hpp
    MSfMDataStats.hpp
  )

# Qt module dependencies
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Qml REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Charts REQUIRED)

# AliceVision dependency
find_package(AliceVision REQUIRED)

# Target properties
add_library(qtAliceVisionPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS} ${PLUGIN_MOCS})

target_link_libraries(qtAliceVisionPlugin
  PUBLIC
    aliceVision_feature
    aliceVision_sfmDataIO
    aliceVision_sfm
    aliceVision_system
    aliceVision_image
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Charts
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
