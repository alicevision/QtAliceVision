#pragma once

#include <Core/IRenderable.hpp>
#include <SfmDataLayer/SfmDataLayer.hpp>
#include <memory>
#include <optional>
#include <vector>

class SfmCameraRenderable : public IRenderable
{
  public:
    void setData(std::vector<SfmDataCameraInstance> cameras, float cameraScale);
    void setSelectedCamera(std::optional<size_t> cameraIndex);

    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void buildGeometry(QRhiResourceUpdateBatch* batch);
    void buildSelectedGeometry(QRhiResourceUpdateBatch* batch);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::vector<SfmDataCameraInstance> _cameras;
    std::optional<size_t> _selectedCameraIndex;
    float _cameraScale = 1.0f;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _selectedVertexBuffer;
    std::unique_ptr<QRhiBuffer> _selectedIndexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    quint32 _indexCount = 0;
    quint32 _selectedIndexCount = 0;

    bool _pipelineDirty = true;
    bool _geometryDirty = true;
    bool _selectedGeometryDirty = true;
};