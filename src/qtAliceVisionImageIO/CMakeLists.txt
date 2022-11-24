# Target srcs
set(PLUGIN_SOURCES
    plugin.cpp
    QtAliceVisionImageIOHandler.cpp
    )

set(PLUGIN_HEADERS
    plugin.hpp
    QtAliceVisionImageIOHandler.hpp
    )


# Qt module dependencies
find_package(Qt5 COMPONENTS Gui REQUIRED)


# OpenImageIO dependency
find_package(OpenImageIO REQUIRED)


# Target properties
add_library(qtAliceVisionImageIOPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

target_include_directories(qtAliceVisionImageIOPlugin 
    PUBLIC 
    ${OPENIMAGEIO_INCLUDE_DIRS}
    )

target_link_libraries(qtAliceVisionImageIOPlugin
    PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
    aliceVision_image
    Qt5::Core
    Qt5::Gui
    )


# Install settings
install(TARGETS qtAliceVisionImageIOPlugin DESTINATION imageformats)
