#pragma once

#include <Core/IRenderable.hpp>
#include <SfmDataLayer/SfmDataLayer.hpp>
#include <Geometry/vertex.hpp>

#include <memory>
#include <QVector4D>
#include <vector>

class SfmPointRenderable : public IRenderable
{
  public:
    void setData(std::vector<SfmDataPointInstance> points, float pointRadius);
    void setPointRadius(float pointRadius);

    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void buildGeometry(QRhiResourceUpdateBatch* batch);
    void rebuildInstanceBuffer(QRhiResourceUpdateBatch* batch);

    // Raw float layout matching std140 exactly: QMatrix4x4 cannot be used
    // here because it carries a hidden internal flagBits member that would
    // misalign spriteInfo when the struct is memcpy'd to the GPU buffer.
    struct UniformData
    {
        float mvp[16];
        float spriteInfo[4];
    };

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::vector<SfmDataPointInstance> _points;
    float _pointRadius = 0.01f;

    std::unique_ptr<QRhiBuffer> _vertexBuffer; /**< Shared unit-quad geometry. */
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _instanceBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    quint32 _instanceCount = 0;

    bool _pipelineDirty = true;
    bool _geomDirty = true;
    bool _instancesDirty = false;
};