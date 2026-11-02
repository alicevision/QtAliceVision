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
 * Selection highlighting is a per-instance scale consumed by the vertex shader, so selecting a
 * camera only requires rewriting a few floats in @ref scalesBuffer, never rebuilding geometry.
 */
struct SfmCameraShapeGpu
{
    std::unique_ptr<QRhiBuffer> vertexBuffer; /**< Unit shape vertices. */
    std::unique_ptr<QRhiBuffer> indexBuffer;  /**< Unit shape line-list indices. */
    quint32 indexCount = 0;                   /**< Number of indices of the unit shape. */

    std::unique_ptr<QRhiBuffer> transformBuffer; /**< Per-instance model matrices. */
    std::unique_ptr<QRhiBuffer> scalesBuffer;    /**< Per-instance scale, enlarged for the selected camera. */
    quint32 instanceCount = 0;                   /**< Number of instances. */

    std::unique_ptr<QRhiBuffer> uniformBuffer;       /**< View-projection uniform. */
    std::unique_ptr<QRhiShaderResourceBindings> srb; /**< Shader resource bindings. */
    std::unique_ptr<QRhiGraphicsPipeline> pipeline;  /**< Instanced line pipeline. */

    QVector<float> transforms; /**< CPU copy of the model matrices (16 column-major floats per instance). */
    QVector<float> scales;     /**< CPU copy of the per-instance scales. */

    std::unordered_map<aliceVision::IndexT, quint32> instanceSlots; /**< viewId -> instance index within this category. */
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
    /**
     * @brief Replace the cameras to draw.
     * @param[in] cameras Cameras, keyed internally by view id.
     */
    void setData(std::vector<SfmDataCameraInstance> cameras);
    /**
     * @brief Set the global camera scale.
     * @param[in] cameraScale Scale applied to every camera shape.
     */
    void setCameraScale(float cameraScale);
    /**
     * @brief Highlight a camera.
     * @param[in] cameraIndex View id of the camera to highlight, or std::nullopt.
     */
    void setSelectedCamera(std::optional<aliceVision::IndexT> cameraIndex);

    /** @brief Store the QRhi and build the pipelines when needed; resets all resources if the QRhi changed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Upload the unit shapes, instances and selection scales when needed, then the view-projection. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the frustum and sphere categories. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Build the pipelines of both shape categories. */
    void buildPipeline();
    /**
     * @brief Create the instanced line pipeline, uniform buffer and SRB of a category.
     * @param[in,out] shape Category to set up.
     */
    void buildShapePipeline(SfmCameraShapeGpu& shape);
    /**
     * @brief Create and upload the unit mesh of a category.
     * @param[in,out] shape Category to set up.
     * @param[in,out] batch Resource update batch receiving the uploads.
     * @param[in] sphereShape True for the sphere wireframe, false for the frustum.
     */
    void buildStaticGeometry(SfmCameraShapeGpu& shape, QRhiResourceUpdateBatch* batch, bool sphereShape);
    /**
     * @brief Split the cameras into categories and compute their per-instance transforms.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void buildInstanceData(QRhiResourceUpdateBatch* batch);
    /**
     * @brief Recreate and upload the instance buffers of a category.
     * @param[in,out] shape Category to upload.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void uploadInstanceBuffers(SfmCameraShapeGpu& shape, QRhiResourceUpdateBatch* batch);
    /**
     * @brief Recompute and upload the per-instance scales of both categories.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void updateSelectionBuffers(QRhiResourceUpdateBatch* batch);
    /**
     * @brief Fill the per-instance scales of a category, enlarging the selected camera.
     * @param[in,out] shape Category to update.
     * @param[in] selected View id of the selected camera, or std::nullopt.
     */
    void applySelectionToFlags(SfmCameraShapeGpu& shape, std::optional<aliceVision::IndexT> selected);
    /**
     * @brief Record the instanced draw of a category.
     * @param[in] cb Command buffer.
     * @param[in] state Per-frame scene state.
     * @param[in,out] shape Category to draw.
     */
    void renderShape(QRhiCommandBuffer* cb, const SceneState& state, SfmCameraShapeGpu& shape);

    /** @brief Rendering hardware interface, set by initialize(). */
    QRhi* _rhi = nullptr;
    /** @brief Render pass descriptor the pipelines are built for. */
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    /** @brief Cameras to draw, by view id. */
    std::unordered_map<aliceVision::IndexT, SfmDataCameraInstance> _cameras;
    /** @brief View id of the highlighted camera. */
    std::optional<aliceVision::IndexT> _selectedCameraIndex;

    /** @brief Global camera scale. */
    float _cameraScale = 1.0f;
    /** @brief Extra scale factor of the highlighted camera. */
    float _selectedScale = 2.0f;

    /** @brief Frustum category (regular cameras). */
    SfmCameraShapeGpu _pyramid;
    /** @brief Sphere category (panoramic cameras). */
    SfmCameraShapeGpu _sphere;

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the unit shapes must be (re)uploaded. */
    bool _staticGeometryDirty = true;
    /** @brief True when the instance data must be rebuilt. */
    bool _instancesDirty = true;
    /** @brief True when the per-instance scales must be uploaded. */
    bool _scaleDirty = true;
};