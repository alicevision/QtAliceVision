# Target srcs
set(PLUGIN_SOURCES
    QtAliceVisionImageIOHandler.cpp
    plugin.cpp
    )

set(PLUGIN_HEADERS
    QtAliceVisionImageIOHandler.hpp
    plugin.hpp
    )

# Qt module dependencies
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)

# comment to handle all possible file formats
# by default: jpeg, png and ico are handled by Qt (for performance reasons)
add_definitions(-DQTOIIO_USE_FORMATS_BLACKLIST)

# OpenImageIO dependency
find_package(OpenImageIO REQUIRED)
if(OPENIMAGEIO_FOUND)
    message(STATUS "OpenImageIO found.")
else()
    message(SEND_ERROR "Failed to find OpenImageIO.")
endif()

add_library(QtOIIOPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

target_include_directories(QtOIIOPlugin
  PUBLIC
    ${OPENIMAGEIO_INCLUDE_DIRS}
    ${OPENEXR_INCLUDE_DIR}
    ${ILMBASE_INCLUDE_DIR}
)
target_link_libraries(QtOIIOPlugin
  PUBLIC
    ${OPENIMAGEIO_LIBRARIES}
)

target_link_libraries(QtOIIOPlugin
  PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
)

install(TARGETS QtOIIOPlugin DESTINATION imageformats)
