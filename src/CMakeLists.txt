# Target srcs
set(PLUGIN_SOURCES FeaturesViewer.cpp)
set(PLUGIN_HEADERS FeaturesViewer.hpp plugin.hpp)

# Target properties
add_library(qtAliceVisionPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

target_link_libraries(qtAliceVisionPlugin
  PUBLIC
    aliceVision_feature
    aliceVision_sfmDataIO
    aliceVision_sfm
    aliceVision_system
    Qt5::Core
    Qt5::Qml
    Qt5::Quick
)

# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/AliceVision)
install(TARGETS qtAliceVisionPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/AliceVision")
