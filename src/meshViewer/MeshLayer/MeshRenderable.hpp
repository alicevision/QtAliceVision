#pragma once

#include <Core/IRenderable.hpp>
#include <MeshLayer/MeshLayer.hpp>
#include <Geometry/vertex.hpp>
#include <memory>

class MeshRenderable : public IRenderable
{
  public:
    void sync(LayerItem* layer, SceneView* view) override;

    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void rebuildBuffers(QRhiResourceUpdateBatch* batch);
    void buildWirePipeline();
    void buildFlatWireVertices(const MeshData& data);
    void rebuildWireBuffer(QRhiResourceUpdateBatch* batch);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _normalPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> _shadedPipeline;

    QVector<Vertex> _vertices;
    QVector<quint32> _indices;

    bool _pipelineDirty = true;
    bool _buffersDirty = true;
    QMatrix4x4 _modelMatrix;

    MeshLayer::ShadingMode _shadingMode = MeshLayer::MeshShaded;
    float _meshOpacity = 1.0f;

    MeshLayer::WireframeMode _wireframeMode = MeshLayer::Solid;
    bool _wireframeDirty = true;
    bool _wireBufferDirty = false;

    QVector<WireVertex> _wireVertices;
    std::unique_ptr<QRhiBuffer> _wireVertexBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _wireSrb;
    std::unique_ptr<QRhiGraphicsPipeline> _wireOverlayPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> _wireOnlyPipeline;
};
