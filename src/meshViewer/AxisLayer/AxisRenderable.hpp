#pragma once

#include <Core/IRenderable.hpp>
#include <Geometry/vertex.hpp>
#include <QMatrix4x4>
#include <memory>

/**
 * @brief IRenderable that draws a world-aligned XYZ axis gizmo at the last pick point.
 *
 * The geometry is static (built once); only the MVP uniform is updated each frame.
 */
class AxisRenderable : public IRenderable
{
  public:
    void sync(LayerItem* layer, SceneView* view) override;
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Creates the simpleColor RHI pipeline and SRB. */
    void buildPipeline();
    /** @brief Builds and uploads the axis arrow geometry. */
    void buildGeometry(QRhiResourceUpdateBatch* batch);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    quint32 _indexCount = 0;
    bool _pipelineDirty = true;
    bool _geomDirty = true;
    QMatrix4x4 _modelMatrix;
};
