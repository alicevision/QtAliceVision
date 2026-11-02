#pragma once

#include <Core/IRenderable.hpp>
#include <MeshLayer/MeshLayer.hpp>
#include <Geometry/vertex.hpp>
#include <MeshLayer/MeshData.hpp>
#include <memory>
#include <vector>

/**
 * @brief IRenderable drawing a MeshLayer.
 *
 * All sub-meshes are merged into a single vertex/index buffer. The solid pass uses the pipeline
 * matching the shading mode; the wireframe uses a separate non-indexed buffer of flat triangles
 * carrying barycentric coordinates.
 */
class MeshRenderable : public IRenderable
{
  public:
    /** @brief Copy the mesh, shading, wireframe and opacity state from the MeshLayer when they changed. */
    void sync(LayerItem* layer, SceneView* view) override;

    /** @brief Store the QRhi and build the pipelines when needed; resets all resources if the QRhi changed. */
    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    /** @brief Rebuild the dirty buffers and material resources, then upload the MVP and opacity. */
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    /** @brief Draw the solid surface and, depending on the wireframe mode, the edges. */
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    /** @brief Create the solid pipelines (normal, shaded and depth-only), uniform buffer and SRB. */
    void buildPipeline();
    /**
     * @brief Recreate and upload the merged vertex and index buffers.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void rebuildBuffers(QRhiResourceUpdateBatch* batch);
    /** @brief Create the wireframe pipelines (overlay and wire-only) and SRB. */
    void buildWirePipeline();
    /**
     * @brief Unroll the triangles into flat vertices with barycentric coordinates.
     * @param[in] data Mesh data.
     */
    void buildFlatWireVertices(const MeshData& data);
    /**
     * @brief Recreate and upload the wireframe vertex buffer.
     * @param[in,out] batch Resource update batch receiving the uploads.
     */
    void rebuildWireBuffer(QRhiResourceUpdateBatch* batch);

    /**
     * @brief Create the GPU resources of the MeshMaterial mode: UV buffer, textures, per-material bindings and pipeline.
     * @param batch Receives the UV and texture uploads.
     * @note Called lazily, only while MeshMaterial shading is active, so other modes never allocate GPU UVs or textures.
     */
    void rebuildMaterialResources(QRhiResourceUpdateBatch* batch);

    /** @brief Release every GPU resource owned by the MeshMaterial mode. */
    void releaseMaterialResources();

    /** @brief Contiguous index range of one sub-mesh inside the merged index buffer. */
    struct DrawRange
    {
        quint32 firstIndex;    /**< First index in the merged index buffer. */
        quint32 indexCount;    /**< Number of indices of the sub-mesh. */
        quint32 firstVertex;   /**< First vertex of the sub-mesh in the merged vertex buffer. */
        int materialIndex;     /**< Index into _materials, or -1 for the default material. */
        QVector<UVVertex> uvs; /**< Sub-mesh UVs, implicitly shared with MeshData (no copy); empty when untextured. */
    };

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
    /** @brief Depth-only pipeline used in WireframeOnly mode to occlude hidden edges. */
    std::unique_ptr<QRhiGraphicsPipeline> _depthPipeline;
    /** @brief Pipeline for MeshNormal shading. */
    std::unique_ptr<QRhiGraphicsPipeline> _normalPipeline;
    /** @brief Pipeline for MeshShaded shading. */
    std::unique_ptr<QRhiGraphicsPipeline> _shadedPipeline;

    /** @brief Merged vertices of all sub-meshes. */
    QVector<Vertex> _vertices;
    /** @brief Merged indices of all sub-meshes. */
    QVector<quint32> _indices;

    /** @brief True when the pipeline must be rebuilt. */
    bool _pipelineDirty = true;
    /** @brief True when the vertex and index buffers must be rebuilt. */
    bool _buffersDirty = true;
    /** @brief Model matrix of the mesh (identity). */
    QMatrix4x4 _modelMatrix;

    /** @brief Current shading mode. */
    MeshLayer::ShadingMode _shadingMode = MeshLayer::MeshShaded;
    /** @brief Current layer opacity. */
    float _meshOpacity = 1.0f;

    /** @brief Current wireframe mode. */
    MeshLayer::WireframeMode _wireframeMode = MeshLayer::Solid;
    /** @brief True when the wireframe vertices must be rebuilt. */
    bool _wireframeDirty = true;
    /** @brief True when the wireframe buffer must be uploaded. */
    bool _wireBufferDirty = false;

    /** @brief Flat wireframe vertices. */
    QVector<WireVertex> _wireVertices;
    /** @brief GPU wireframe vertex buffer. */
    std::unique_ptr<QRhiBuffer> _wireVertexBuffer;
    /** @brief Shader resource bindings of the wireframe pipelines. */
    std::unique_ptr<QRhiShaderResourceBindings> _wireSrb;
    /** @brief Wireframe pipeline drawn over the surface. */
    std::unique_ptr<QRhiGraphicsPipeline> _wireOverlayPipeline;
    /** @brief Wireframe pipeline drawn without the surface. */
    std::unique_ptr<QRhiGraphicsPipeline> _wireOnlyPipeline;

    /** @name MeshMaterial mode
     *  CPU data is copied from MeshData on mesh change; GPU resources are created on first use of the mode.
     *  @{ */
    /** @brief Index range and material of each sub-mesh. */
    QVector<DrawRange> _drawRanges;
    /** @brief Copy of the mesh materials. */
    QVector<MeshMaterial> _materials;
    /** @brief Texture images waiting for upload. */
    QVector<std::shared_ptr<const MeshTextureImage>> _textureImages;
    bool _meshHasUVs = false;     /**< True when at least one sub-mesh is textured, i.e. MeshMaterial can be drawn. */
    bool _materialsDirty = false; /**< GPU material resources must be (re)built before the next MeshMaterial draw. */

    /** @brief GPU texture coordinate buffer. */
    std::unique_ptr<QRhiBuffer> _uvBuffer;
    /** @brief Sampler shared by all material textures. */
    std::unique_ptr<QRhiSampler> _materialSampler;
    std::unique_ptr<QRhiTexture> _whiteTexture; /**< Bound for untextured materials. */
    /** @brief GPU textures, parallel to _textureImages. */
    std::vector<std::unique_ptr<QRhiTexture>> _textures;
    std::vector<std::unique_ptr<QRhiBuffer>> _materialBuffers;              /**< One base color UBO per material, plus a trailing default one. */
    std::vector<std::unique_ptr<QRhiShaderResourceBindings>> _materialSrbs; /**< Parallel to _materialBuffers. */
    /** @brief Pipeline for MeshMaterial shading. */
    std::unique_ptr<QRhiGraphicsPipeline> _materialPipeline;
    /** @} */
};
