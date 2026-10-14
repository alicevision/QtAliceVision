# Target srcs
set(PLUGIN_SOURCES
    CurveModel.cpp
    )

set(PLUGIN_HEADERS
    CurveModel.hpp
    )


# Qt module dependency
find_package(Qt6 COMPONENTS Core Gui Qml REQUIRED)


# Target properties
add_library(curveViewerPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

if(MSVC)
    target_compile_options(curveViewerPlugin PUBLIC /W4)
else()
    target_compile_options(curveViewerPlugin PUBLIC -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic)
endif()

target_link_libraries(curveViewerPlugin
  PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
)

set_target_properties(curveViewerPlugin
        PROPERTIES
        DEBUG_POSTFIX ""
        FOLDER "curveViewerPlugin"
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        )
