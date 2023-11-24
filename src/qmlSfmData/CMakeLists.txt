# Target srcs
set(PLUGIN_SOURCES
    IOThread.cpp
    SfmDataEntity.cpp
    CameraLocatorEntity.cpp
    PointCloudEntity.cpp
)

set(PLUGIN_HEADERS
    IOThread.hpp
    plugin.hpp
    SfmDataEntity.hpp
    CameraLocatorEntity.hpp
    PointCloudEntity.hpp
)

# Qt module dependency
find_package(Qt5 COMPONENTS Qml REQUIRED)
find_package(Qt5 COMPONENTS 3DCore REQUIRED)
find_package(Qt5 COMPONENTS 3DRender REQUIRED)
find_package(Qt5 COMPONENTS 3DExtras REQUIRED)

# Target properties
add_library(sfmDataEntityQmlPlugin SHARED ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

if(MSVC)
    target_compile_options(sfmDataEntityQmlPlugin PUBLIC /W4)
else()
    target_compile_options(sfmDataEntityQmlPlugin
        PUBLIC
        -Wall -Wextra -Wconversion -Wsign-conversion -Wshadow -Wpedantic
    )
endif()

target_link_libraries(sfmDataEntityQmlPlugin
    PUBLIC
        Qt5::Core
        Qt5::Qml
        Qt5::3DCore
        Qt5::3DRender
        aliceVision_sfmDataIO
        aliceVision_sfm
        aliceVision_system
    PRIVATE
        Qt5::3DExtras
)

set_target_properties(sfmDataEntityQmlPlugin
    PROPERTIES
    DEBUG_POSTFIX ""
    FOLDER "sfmDataEntityQmlPlugin"
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

# Install settings
install(FILES "qmldir"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/SfmDataEntity)
install(TARGETS sfmDataEntityQmlPlugin
        DESTINATION "${CMAKE_INSTALL_PREFIX}/qml/SfmDataEntity")
