#pragma once

#include <Core/IRenderable.hpp>
#include <Geometry/vertex.hpp>
#include <QMatrix4x4>
#include <QVector>
#include <memory>

/** @brief IRenderable drawing the DepthmapLayer mesh with per-vertex colors. */
class DepthmapRenderable : public IRenderable
{
  public:
    /** @brief Copy the mesh data from the DepthmapLayer when it changed. */
    void sync(LayerItem* layer, SceneView* view) override;
    /** @brief Store the QRhi and build the pipeline when needed; resets all resources if the QRhi changed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Upload the mesh when it changed, then the view-projection matrix. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the mesh. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Create the colored-triangle pipeline, uniform buffer and SRB. */
    void buildPipeline();
    /**
     * @brief Recreate and upload the vertex and index buffers.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void rebuildBuffers(QRhiResourceUpdateBatch* batch);

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

    /** @brief CPU copy of the vertices. */
    QVector<ColoredVertex> _vertices;
    /** @brief CPU copy of the triangle indices. */
    QVector<quint32> _indices;
    /** @brief Number of indices uploaded to the GPU. */
    quint32 _indexCount = 0;

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the vertex and index buffers must be rebuilt. */
    bool _buffersDirty = true;
};
