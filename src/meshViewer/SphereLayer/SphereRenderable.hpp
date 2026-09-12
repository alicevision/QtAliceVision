#pragma once

#include <Core/IRenderable.hpp>

#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include <vector>

#include <Geometry/vertex.hpp>

class SphereRenderable : public IRenderable
{
  public:
    void sync(LayerItem* layer, SceneView* view) override;
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void setPositions(const std::vector<QVector3D>& positions);
    void buildPipeline();
    void buildGeometry(QRhiResourceUpdateBatch* batch);
    void rebuildInstanceBuffer(QRhiResourceUpdateBatch* batch);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::vector<QVector3D> _positions;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _instanceBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    quint32 _indexCount = 0;
    quint32 _instanceCount = 0;

    bool _pipelineDirty = true;
    bool _geomDirty = true;
    bool _instancesDirty = false;
};
