#pragma once

#include <Core/IRenderable.hpp>
#include <Geometry/vertex.hpp>
#include <QMatrix4x4>
#include <QVector>
#include <memory>

class DepthmapRenderable : public IRenderable
{
  public:
    void sync(LayerItem* layer, SceneView* view) override;
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void rebuildBuffers(QRhiResourceUpdateBatch* batch);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    QVector<ColoredVertex> _vertices;
    QVector<quint32> _indices;
    quint32 _indexCount = 0;

    bool _pipelineDirty = true;
    bool _buffersDirty = true;
};
