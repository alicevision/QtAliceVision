#pragma once

#include <Core/IRenderable.hpp>
#include <SfmDataLayer/SfmDataLayer.hpp>
#include <Geometry/vertex.hpp>

#include <memory>
#include <QVector4D>
#include <vector>

/** @brief IRenderable drawing SfM landmarks as instanced screen-space sprites. */
class SfmPointRenderable : public IRenderable
{
  public:
    /**
     * @brief Replace the landmarks.
     * @param[in] points Landmarks.
     * @param[in] pointRadius Sprite size, in pixels.
     */
    void setData(std::vector<SfmDataPointInstance> points, float pointRadius);
    /**
     * @brief Set the sprite size.
     * @param[in] pointRadius Size, in pixels.
     */
    void setPointRadius(float pointRadius);

    /** @brief Store the QRhi and build the pipeline when needed; resets all resources if the QRhi changed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Upload the quad and instances when needed, then the uniforms. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw all landmark sprites. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Create the pointSprite pipeline, uniform buffer and SRB. */
    void buildPipeline();
    /**
     * @brief Create and upload the sprite quad.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void buildGeometry(QRhiResourceUpdateBatch* batch);
    /**
     * @brief Recreate and upload the per-instance position and color buffer.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void rebuildInstanceBuffer(QRhiResourceUpdateBatch* batch);

    /**
     * @brief Uniform block of the pointSprite shaders.
     *
     * Raw float layout matching std140 exactly: QMatrix4x4 cannot be used here because it
     * carries a hidden internal flagBits member that would misalign spriteInfo when the struct
     * is memcpy'd to the GPU buffer.
     */
    struct UniformData
    {
        float mvp[16];       /**< View-projection matrix. */
        float spriteInfo[4]; /**< Point size in pixels, viewport width, viewport height, padding. */
    };

    /** @brief Rendering hardware interface, set by initialize(). */
    QRhi* _rhi = nullptr;
    /** @brief Render pass descriptor the pipelines are built for. */
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    /** @brief Landmarks to draw. */
    std::vector<SfmDataPointInstance> _points;
    /** @brief Sprite size, in pixels. */
    float _pointRadius = 0.01f;

    /** @brief GPU vertex buffer. */
    std::unique_ptr<QRhiBuffer> _vertexBuffer; /**< Shared unit-quad geometry. */
    /** @brief GPU index buffer. */
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    /** @brief GPU per-instance position and color buffer. */
    std::unique_ptr<QRhiBuffer> _instanceBuffer;
    /** @brief Per-frame uniform buffer. */
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    /** @brief Shader resource bindings of the pipeline. */
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    /** @brief Graphics pipeline. */
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    /** @brief Number of sprites to draw. */
    quint32 _instanceCount = 0;

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the quad must be (re)uploaded. */
    bool _geomDirty = true;
    /** @brief True when the instance buffer must be rebuilt. */
    bool _instancesDirty = false;
};