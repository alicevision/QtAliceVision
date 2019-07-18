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
