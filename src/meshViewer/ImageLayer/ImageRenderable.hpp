#pragma once

#include <Core/IRenderable.hpp>
#include <aliceVision/image/Image.hpp>
#include <memory>

class ImageRenderable : public IRenderable
{
  public:
    void sync(LayerItem* layer, SceneView* view) override;
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void buildGeometry(QRhiResourceUpdateBatch* batch);
    void uploadTexture(QRhiResourceUpdateBatch* batch);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiTexture> _texture;
    std::unique_ptr<QRhiSampler> _sampler;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    aliceVision::image::Image<aliceVision::image::RGBAfColor> _image;
    bool _pipelineDirty = true;
    bool _geomDirty = false;
    bool _textureDirty = false;
};
