#pragma once

#include <Core/IRenderable.hpp>
#include <aliceVision/image/Image.hpp>
#include <memory>

/**
 * @brief IRenderable drawing the ImageLayer image as a screen-aligned quad.
 *
 * The quad is placed with SceneState::orthogonalMatrix and the fragment shader samples the image
 * through the undistortion map.
 */
class ImageRenderable : public IRenderable
{
  public:
    /** @brief Take the newly loaded image and map from the ImageLayer, if any. */
    void sync(LayerItem* layer, SceneView* view) override;
    /** @brief Store the QRhi; the pipeline is built once an image is available. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Build the pipeline, geometry and textures when needed, then upload the image transform. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the image quad. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Create the imageQuad pipeline, textures, samplers and SRB sized for the current image. */
    void buildPipeline();
    /**
     * @brief Create and upload the full-screen quad.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void buildGeometry(QRhiResourceUpdateBatch* batch);
    /**
     * @brief Upload the image and map textures, recreating them if the size changed.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void uploadTexture(QRhiResourceUpdateBatch* batch);

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

    /** @brief Image texture. */
    std::unique_ptr<QRhiTexture> _textureImage;
    /** @brief Undistortion map texture. */
    std::unique_ptr<QRhiTexture> _textureMap;
    /** @brief Image sampler. */
    std::unique_ptr<QRhiSampler> _samplerImage;
    /** @brief Undistortion map sampler. */
    std::unique_ptr<QRhiSampler> _samplerMap;

    /** @brief Shader resource bindings of the pipeline. */
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    /** @brief Graphics pipeline. */
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    /** @brief CPU copy of the image. */
    aliceVision::image::Image<aliceVision::image::RGBAfColor> _image;
    /** @brief CPU copy of the undistortion map. */
    aliceVision::image::Image<aliceVision::image::RGBAfColor> _map;

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the quad must be (re)uploaded. */
    bool _geomDirty = false;
    /** @brief True when the textures must be (re)uploaded. */
    bool _textureDirty = false;
};
