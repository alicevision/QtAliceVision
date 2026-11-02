#pragma once

#include <Core/IRenderable.hpp>
#include <Geometry/vertex.hpp>

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector>
#include <QVector3D>
#include <memory>

/**
 * @brief IRenderable drawing a BoxControlLayer: translucent box faces, box edges and the transform gizmo.
 *
 * The gizmo is drawn on top of the scene.
 * The box geometry is static (unit cube, transformed by the box matrix).
 * The gizmo geometry is rebuilt when the highlighted handle, the box scale or the gizmo screen scale changes,
 * since the face handles sit at a world distance from the box faces.
 */
class BoxControlRenderable : public IRenderable
{
  public:
    /** @brief Copy the box, gizmo and highlight state from the BoxControlLayer. */
    void sync(LayerItem* layer, SceneView* view) override;
    /** @brief Store the QRhi and build the pipelines when needed; resets all resources if the QRhi changed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Upload the box geometry once, rebuild the gizmo when needed, then update the uniforms. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the box faces and edges, then the gizmo on top. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief One draw: geometry buffers, uniform buffer, bindings and pipeline. */
    struct DrawItem
    {
        std::unique_ptr<QRhiBuffer> vertexBuffer;            /**< Vertex buffer. */
        std::unique_ptr<QRhiBuffer> indexBuffer;             /**< Index buffer, or null for non-indexed draws. */
        std::unique_ptr<QRhiBuffer> uniformBuffer;           /**< MVP and alpha uniforms. */
        std::unique_ptr<QRhiShaderResourceBindings> srb;     /**< Shader resource bindings. */
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;      /**< Graphics pipeline. */
        quint32 count = 0;                                   /**< Number of indices, or vertices when not indexed. */
        /** @brief Upper bound of the viewport depth range used for this draw. */
        float maxDepth = 1.f;
    };

    /** @brief Create the faces, edges and gizmo pipelines with their uniform buffers and SRBs. */
    void buildPipelines();
    /**
     * @brief Create and upload the unit cube faces and edges.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void buildBoxGeometry(QRhiResourceUpdateBatch* batch);
    /** @brief Builds the gizmo mesh in gizmo units, with face handles placed for @p gizmoScale. */
    void buildGizmoMesh(QVector<ColoredVertex>& verts, QVector<quint32>& indices, float gizmoScale) const;
    /**
     * @brief Rebuild and upload the gizmo mesh if its inputs changed, growing the buffers when needed.
     * @param[in,out] batch Resource update batch receiving the uploads.
     * @param[in] gizmoScale World size of one gizmo unit.
     */
    void updateGizmoGeometry(QRhiResourceUpdateBatch* batch, float gizmoScale);
    /**
     * @brief Upload the MVP and alpha of a draw.
     * @param[in,out] batch Resource update batch receiving the upload.
     * @param[in,out] item Draw to update.
     * @param[in] mvp Model-view-projection matrix.
     * @param[in] alpha Opacity.
     */
    void updateUniforms(QRhiResourceUpdateBatch* batch, DrawItem& item, const QMatrix4x4& mvp, float alpha);
    /** @brief Records @p item into @p cb, indexed when it has an index buffer. */
    void draw(QRhiCommandBuffer* cb, const SceneState& state, DrawItem& item);

    /** @brief Rendering hardware interface, set by initialize(). */
    QRhi* _rhi = nullptr;
    /** @brief Render pass descriptor the pipelines are built for. */
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    /** @brief Translucent box faces. */
    DrawItem _faces;
    /** @brief Box edges. */
    DrawItem _edges;
    /** @brief Transform gizmo. */
    DrawItem _gizmo;

    /** @name Layer state copied in sync()
     *  @{ */
    /** @brief Box center. */
    QVector3D _translation;
    /** @brief Box orientation. */
    QQuaternion _orientation;
    /** @brief Box half extents. */
    QVector3D _scale = QVector3D(1.f, 1.f, 1.f);
    /** @brief Gizmo size, as a fraction of the viewport half height. */
    float _gizmoSize = 0.15f;
    /** @brief Gizmo visibility. */
    bool _gizmoVisible = true;
    /** @brief Highlighted handle. */
    int _highlightedHandle = 0;
    /** @} */

    /** @name Gizmo state the current gizmo geometry was built for
     *  @{ */
    /** @brief Gizmo scale of the current geometry. */
    float _builtGizmoScale = -1.f;
    /** @brief Box scale of the current geometry. */
    QVector3D _builtScale;
    /** @brief Highlighted handle of the current geometry. */
    int _builtHighlight = -1;
    /** @brief Vertex capacity of the gizmo buffers. */
    quint32 _gizmoVertexCapacity = 0;
    /** @} */

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the box geometry must be (re)uploaded. */
    bool _boxGeomDirty = true;
};
