#pragma once

#include <Core/IRenderable.hpp>
#include <Geometry/vertex.hpp>
#include <QMatrix4x4>
#include <memory>

/**
 * @brief IRenderable that draws a world-aligned XYZ axis gizmo at the camera orbit center.
 *
 * The geometry is static (built once); only the MVP uniform is updated each frame.
 */
class AxisRenderable : public IRenderable
{
  public:
    /** @brief Copy the orbit center from the view MotionInfo into the model matrix. */
    void sync(LayerItem* layer, SceneView* view) override;
    /** @brief Store the QRhi and build the pipeline when needed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Upload the geometry once, then the MVP scaled to keep a constant screen size. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the gizmo. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Creates the simpleColor RHI pipeline and SRB. */
    void buildPipeline();
    /** @brief Builds and uploads the axis arrow geometry. */
    void buildGeometry(QRhiResourceUpdateBatch* batch);

    /** @brief Rendering hardware interface, set by initialize(). */
    QRhi* _rhi = nullptr;
    /** @brief Render pass descriptor the pipelines are built for. */
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    /** @brief GPU vertex buffer. */
    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    /** @brief GPU index buffer. */
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    /** @brief Per-frame uniform buffer. */
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    /** @brief Shader resource bindings of the pipeline. */
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    /** @brief Graphics pipeline. */
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    /** @brief Number of indices of the gizmo mesh. */
    quint32 _indexCount = 0;
    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the geometry must be (re)uploaded. */
    bool _geomDirty = true;
    /** @brief Translation of the gizmo to the orbit center. */
    QMatrix4x4 _modelMatrix;
};
