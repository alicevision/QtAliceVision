find_package(assimp REQUIRED)
find_package(embree REQUIRED)
find_package(meshoptimizer REQUIRED)
find_package(OpenImageIO REQUIRED)

qt_add_qml_module(meshViewer
    URI meshViewer
    VERSION 1.0
    PLUGIN_TARGET meshViewer
    NO_PLUGIN_OPTIONAL
    SOURCES

        Core/BoundingBox.hpp
        Core/CameraInfo.hpp
        Core/CameraInfo.cpp
        Core/Picking.hpp
        Core/Picking.cpp
        Core/MotionInfo.hpp
        Core/MotionInfo.cpp
        Core/IRenderable.hpp
        Core/LayerItem.hpp
        Core/LayerItem.cpp
        Core/SceneView.hpp
        Core/SceneView.cpp
        Core/SceneRenderer.hpp
        Core/SceneRenderer.cpp
        Core/SceneState.hpp
        Core/ShaderLoader.hpp
        
        Camera/OrbitMotionInfo.hpp
        Camera/OrbitMotionInfo.cpp
        Camera/BaseCameraInfo.hpp
        Camera/BaseCameraInfo.cpp

)

set(meshViewer_QMLTYPEREGISTRATIONS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Camera
)

set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/meshviewer_qmltyperegistrations.cpp
    PROPERTIES INCLUDE_DIRECTORIES "${meshViewer_QMLTYPEREGISTRATIONS_INCLUDE_DIRS}"
)

# RHI headers live in GuiPrivate
target_link_libraries(meshViewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::GuiPrivate
    Qt6::Quick
    Qt6::ShaderTools
    Qt6::Concurrent
    assimp::assimp
    embree
    meshoptimizer::meshoptimizer
    ${OPENIMAGEIO_LIBRARIES}
    aliceVision_image
    aliceVision_sfmData
    aliceVision_sfmDataIO
    aliceVision_geometry
    aliceVision_mvsData
    aliceVision_numeric
)

target_include_directories(meshViewer PRIVATE ${OPENIMAGEIO_INCLUDE_DIRS})

# Shaders
qt_add_shaders(meshViewer "mesh_shaders"
    GLSL "460"
    PREFIX "/shaders"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    FILES
        shaders/simpleColor.vert
        shaders/simpleColor.frag
)

# Install the plugin shared library
install(TARGETS meshViewer
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/meshViewer
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/meshViewer
)

# Install the generated qmldir
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/qmldir"
    DESTINATION ${CMAKE_INSTALL_PREFIX}/qml/meshViewer
)
