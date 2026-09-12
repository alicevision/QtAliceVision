find_package(assimp REQUIRED)
find_package(embree REQUIRED)
find_package(meshoptimizer REQUIRED)

qt_add_qml_module(meshViewer
    URI meshViewer
    VERSION 1.0
    PLUGIN_TARGET meshViewer
    NO_PLUGIN_OPTIONAL
    SOURCES

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
        
        Camera/OrbitMotionInfo.hpp
        Camera/OrbitMotionInfo.cpp
        Camera/AVCameraInfo.hpp
        Camera/AVCameraInfo.cpp
        
        MeshLayer/MeshObject.hpp
        MeshLayer/MeshObject.cpp
        MeshLayer/MeshLayer.hpp
        MeshLayer/MeshLayer.cpp
        MeshLayer/MeshRenderable.hpp
        MeshLayer/MeshRenderable.cpp
        MeshLayer/MeshLoader.hpp
        MeshLayer/MeshLoader.cpp
        MeshLayer/MeshPicker.hpp
        MeshLayer/MeshPicker.cpp
        
        AxisLayer/AxisLayer.hpp
        AxisLayer/AxisGeometry.hpp
        AxisLayer/AxisRenderable.hpp
        AxisLayer/AxisRenderable.cpp

        SphereLayer/SphereLayer.hpp
        SphereLayer/SphereRenderable.hpp
        SphereLayer/SphereRenderable.cpp

        SfmDataLayer/SfmDataObject.hpp
        SfmDataLayer/SfmDataObject.cpp
        SfmDataLayer/SfmDataLayer.hpp
        SfmDataLayer/SfmDataLayer.cpp
        SfmDataLayer/SfmDataPicker.hpp
        SfmDataLayer/SfmDataPicker.cpp
        SfmDataLayer/SfmDataRenderable.hpp
        SfmDataLayer/SfmDataRenderable.cpp
        SfmDataLayer/SfmPointRenderable.hpp
        SfmDataLayer/SfmPointRenderable.cpp
        SfmDataLayer/SfmCameraRenderable.hpp
        SfmDataLayer/SfmCameraRenderable.cpp
        
        ImageLayer/ImageLayer.hpp
        ImageLayer/ImageLayer.cpp
        ImageLayer/ImageRenderable.hpp
        ImageLayer/ImageRenderable.cpp

        GridLayer/GridLayer.hpp
        GridLayer/GridRenderable.hpp
        GridLayer/GridRenderable.cpp
)

set(meshViewer_QMLTYPEREGISTRATIONS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/Core
    ${CMAKE_CURRENT_SOURCE_DIR}/Camera
    ${CMAKE_CURRENT_SOURCE_DIR}/AxisLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/GridLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/MeshLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/SphereLayer
    ${CMAKE_CURRENT_SOURCE_DIR}/SfmDataLayer
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
    aliceVision_image
    aliceVision_sfmData
    aliceVision_sfmDataIO
    aliceVision_geometry
)

# Shaders
qt_add_shaders(meshViewer "mesh_shaders"
    GLSL "460"
    PREFIX "/shaders"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
    FILES
        shaders/meshShaded.vert
        shaders/meshShaded.frag
        shaders/meshNormal.vert
        shaders/meshNormal.frag
        shaders/meshWire.vert
        shaders/meshWire.frag
        shaders/pointSprite.vert
        shaders/pointSprite.frag
        shaders/simpleColor.vert
        shaders/simpleColor.frag
        shaders/instancedColor.vert
        shaders/imageQuad.vert
        shaders/imageQuad.frag
        shaders/gridLayer.vert
        shaders/gridLayer.frag
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