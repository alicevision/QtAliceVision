#pragma once

#include <Core/IRenderable.hpp>
#include <memory>

class GridRenderable : public IRenderable
{
  public:
    void sync(LayerItem* layer, SceneView* view) override;
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void buildGeometry(QRhiResourceUpdateBatch* batch);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    float _baseSpacing = 0.1f;
    float _subdivisions = 10.0f;
    float _majorLineWidth = 1.25f;
    float _minorLineWidth = 1.0f;
    float _minorOpacity = 0.35f;
    float _minCellPixels = 8.0f;

    bool _pipelineDirty = true;
    bool _geomDirty = false;
};
