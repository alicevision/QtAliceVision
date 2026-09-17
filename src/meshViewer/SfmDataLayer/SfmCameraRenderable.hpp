#pragma once

#include <Core/IRenderable.hpp>
#include <SfmDataLayer/SfmDataLayer.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include <aliceVision/types.hpp>

#include <QVector>

/**
 * @brief GPU-instanced geometry, GPU resources and CPU-side instance data for one camera "shape"
 *        category (either the frustum pyramid used for normal-FOV cameras, or the sphere
 *        wireframe used for panoramic (FOV > 180 deg) cameras).
 *
 * The static vertex/index buffers hold a single canonical "unit" mesh built once. Each camera is
 * rendered as one GPU instance whose per-instance transform (rotation, translation and
 * non-uniform scale derived from that camera's intrinsics) reproduces its actual frustum/sphere.
 * Selection highlighting is a per-instance flag consumed by the vertex shader, so selecting a
 * camera only requires rewriting a few floats in @ref selectedBuffer, never rebuilding geometry.
 */
struct SfmCameraShapeGpu
{
    std::unique_ptr<QRhiBuffer> vertexBuffer;
    std::unique_ptr<QRhiBuffer> indexBuffer;
    quint32 indexCount = 0;

    std::unique_ptr<QRhiBuffer> transformBuffer;
    std::unique_ptr<QRhiBuffer> scalesBuffer;
    quint32 instanceCount = 0;

    std::unique_ptr<QRhiBuffer> uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> srb;
    std::unique_ptr<QRhiGraphicsPipeline> pipeline;

    QVector<float> transforms;
    QVector<float> scales;

    std::unordered_map<aliceVision::IndexT, quint32> instanceSlots;  ///< viewId -> instance index within this category.
};

/**
 * @brief Renders sfmData cameras as GPU-instanced wireframe shapes.
 *
 * Cameras are grouped into two shape categories (frustum pyramid, or sphere for panoramic
 * cameras), each drawn with a single instanced draw call from a canonical unit-shape mesh built
 * once. Runtime camera selection is resolved by the vertex shader from a small per-instance flag,
 * so it never requires rebuilding geometry.
 */
class SfmCameraRenderable : public IRenderable
{
  public:
    void setData(std::vector<SfmDataCameraInstance> cameras);
    void setCameraScale(float cameraScale);
    void setSelectedCamera(std::optional<aliceVision::IndexT> cameraIndex);

    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void buildShapePipeline(SfmCameraShapeGpu& shape);
    void buildStaticGeometry(SfmCameraShapeGpu& shape, QRhiResourceUpdateBatch* batch, bool sphereShape);
    void buildInstanceData(QRhiResourceUpdateBatch* batch);
    void uploadInstanceBuffers(SfmCameraShapeGpu& shape, QRhiResourceUpdateBatch* batch);
    void updateSelectionBuffers(QRhiResourceUpdateBatch* batch);
    void applySelectionToFlags(SfmCameraShapeGpu& shape, std::optional<aliceVision::IndexT> selected);
    void renderShape(QRhiCommandBuffer* cb, const SceneState& state, SfmCameraShapeGpu& shape);

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::unordered_map<aliceVision::IndexT, SfmDataCameraInstance> _cameras;
    std::optional<aliceVision::IndexT> _selectedCameraIndex;

    float _cameraScale = 1.0f;
    float _selectedScale = 2.0f;

    SfmCameraShapeGpu _pyramid;
    SfmCameraShapeGpu _sphere;

    bool _pipelineDirty = true;
    bool _staticGeometryDirty = true;
    bool _instancesDirty = true;
    bool _scaleDirty = true;
};