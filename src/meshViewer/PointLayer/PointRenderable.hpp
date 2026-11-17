#pragma once

#include <Core/IRenderable.hpp>

#include <QColor>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include <vector>

#include <Geometry/vertex.hpp>

/** @brief IRenderable drawing the PointLayer spheres with GPU instancing (one instance per position). */
class PointRenderable : public IRenderable
{
  public:
    /** @brief Copy the positions, selection, colors and size parameters from the PointLayer when they changed. */
    void sync(LayerItem* layer, SceneView* view) override;
    /** @brief Store the QRhi and build the pipeline when needed; resets all resources if the QRhi changed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Upload the sphere mesh and instances when needed, then the per-frame uniforms. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw all sphere instances. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /**
     * @brief Build the per-instance data (center + color) and flag the instance buffer dirty.
     * @param[in] positions Sphere centers, in world coordinates.
     * @param[in] selectedIndex Index of the sphere drawn with @p selectedColor, -1 for none.
     * @param[in] color Color of the unselected spheres.
     * @param[in] selectedColor Color of the selected sphere.
     */
    void setInstances(const std::vector<QVector3D>& positions, int selectedIndex, const QColor& color, const QColor& selectedColor);
    /** @brief Create the instanced pipeline, uniform buffer and SRB. */
    void buildPipeline();
    /**
     * @brief Create and upload the unit sphere mesh.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void buildGeometry(QRhiResourceUpdateBatch* batch);
    /**
     * @brief Recreate and upload the per-instance buffer (center + color).
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void rebuildInstanceBuffer(QRhiResourceUpdateBatch* batch);

    /** @brief Rendering hardware interface, set by initialize(). */
    QRhi* _rhi = nullptr;
    /** @brief Render pass descriptor the pipelines are built for. */
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    /** @brief Per-instance data: sphere center (world coordinates) and color. */
    std::vector<ColoredVertex> _instances;

    /** @brief GPU vertex buffer. */
    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    /** @brief GPU index buffer. */
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    /** @brief GPU per-instance buffer (center + color). */
    std::unique_ptr<QRhiBuffer> _instanceBuffer;
    /** @brief Per-frame uniform buffer. */
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    /** @brief Shader resource bindings of the pipeline. */
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    /** @brief Graphics pipeline. */
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;

    /** @brief Number of indices of the sphere mesh. */
    quint32 _indexCount = 0;
    /** @brief Number of instances to draw. */
    quint32 _instanceCount = 0;

    /** @brief Sphere radius. */
    float _size = 0.2f;
    /** @brief Constant screen size flag. */
    bool _fixedSize = false;

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the sphere mesh must be (re)uploaded. */
    bool _geomDirty = true;
    /** @brief True when the instance buffer must be rebuilt. */
    bool _instancesDirty = false;
};
