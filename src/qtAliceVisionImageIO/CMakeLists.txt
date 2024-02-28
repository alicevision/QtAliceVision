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
find_package(Qt6 COMPONENTS Gui REQUIRED)


# OpenImageIO dependency
find_package(OpenImageIO REQUIRED)


# Target properties
add_library(qtAliceVisionImageIOPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

if(MSVC)
    target_compile_options(qtAliceVisionImageIOPlugin PUBLIC /W4)
else()
    target_compile_options(qtAliceVisionImageIOPlugin
        PUBLIC
        -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic
        )
endif()

target_include_directories(qtAliceVisionImageIOPlugin 
    PUBLIC 
    ${OPENIMAGEIO_INCLUDE_DIRS}
    )

target_link_libraries(qtAliceVisionImageIOPlugin
    PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
    aliceVision_image
    Qt6::Core
    Qt6::Gui
    )


# Install settings
install(TARGETS qtAliceVisionImageIOPlugin DESTINATION imageformats)
