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
    void sync(LayerItem* layer, SceneView* view) override;
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief One draw: geometry buffers, uniform buffer, bindings and pipeline. */
    struct DrawItem
    {
        std::unique_ptr<QRhiBuffer> vertexBuffer;
        std::unique_ptr<QRhiBuffer> indexBuffer;
        std::unique_ptr<QRhiBuffer> uniformBuffer;
        std::unique_ptr<QRhiShaderResourceBindings> srb;
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        quint32 count = 0;
    };

    void buildPipelines();
    void buildBoxGeometry(QRhiResourceUpdateBatch* batch);
    /** @brief Builds the gizmo mesh in gizmo units, with face handles placed for @p gizmoScale. */
    void buildGizmoMesh(QVector<ColoredVertex>& verts, QVector<quint32>& indices, float gizmoScale) const;
    void updateGizmoGeometry(QRhiResourceUpdateBatch* batch, float gizmoScale);
    void updateUniforms(QRhiResourceUpdateBatch* batch, DrawItem& item, const QMatrix4x4& mvp, float alpha);
    void draw(QRhiCommandBuffer* cb, const SceneState& state, DrawItem& item, bool indexed, float maxDepth = 1.f);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    DrawItem _faces;
    DrawItem _edges;
    DrawItem _gizmo;

    // Layer state copied in sync()
    QVector3D _translation;
    QQuaternion _orientation;
    QVector3D _scale = QVector3D(1.f, 1.f, 1.f);
    float _gizmoSize = 0.15f;
    bool _gizmoVisible = true;
    int _highlightedHandle = 0;

    // Gizmo state the current gizmo geometry was built for
    float _builtGizmoScale = -1.f;
    QVector3D _builtScale;
    int _builtHighlight = -1;
    quint32 _gizmoVertexCapacity = 0;

    bool _pipelineDirty = true;
    bool _boxGeomDirty = true;
};
