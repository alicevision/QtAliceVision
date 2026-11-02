#pragma once

#include <Core/IRenderable.hpp>
#include <memory>

/**
 * @brief IRenderable drawing the GridLayer as a full-screen quad.
 *
 * The fragment shader intersects each pixel ray with the y = 0 plane, using the inverse
 * view-projection matrix, and computes the anti-aliased grid lines analytically.
 */
class GridRenderable : public IRenderable
{
  public:
    /** @brief Copy the grid settings from the GridLayer. */
    void sync(LayerItem* layer, SceneView* view) override;
    /** @brief Store the QRhi and build the pipeline when needed; resets all resources if the QRhi changed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Upload the quad once, then the grid uniforms. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the full-screen quad. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Create the gridLayer pipeline, uniform buffer and SRB. */
    void buildPipeline();
    /**
     * @brief Create and upload the full-screen quad.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void buildGeometry(QRhiResourceUpdateBatch* batch);

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
    /** @brief Shader resource bindings of the pipeline. */
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    /** @brief Graphics pipeline. */
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    /** @brief Finest grid spacing, in world units. */
    float _baseSpacing = 0.1f;
    /** @brief Ratio between two consecutive grid levels. */
    float _subdivisions = 10.0f;
    /** @brief Major line width, in pixels. */
    float _majorLineWidth = 1.25f;
    /** @brief Minor line width, in pixels. */
    float _minorLineWidth = 1.0f;
    /** @brief Minor line opacity. */
    float _minorOpacity = 0.35f;
    /** @brief Cell size below which a level is fully faded out, in pixels. */
    float _minCellPixels = 8.0f;

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the quad must be (re)uploaded. */
    bool _geomDirty = false;
};
