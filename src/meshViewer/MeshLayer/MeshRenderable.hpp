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
    /** @brief Contiguous index range of one sub-mesh inside the merged index buffer. */
    struct DrawRange
    {
        quint32 firstIndex;    /**< First index in the merged index buffer. */
        quint32 indexCount;    /**< Number of indices of the sub-mesh. */
        quint32 firstVertex;   /**< First vertex of the sub-mesh in the merged vertex buffer. */
        int materialIndex;     /**< Index into _materials, or -1 for the default material. */
        QVector<UVVertex> uvs; /**< Sub-mesh UVs, implicitly shared with MeshData (no copy); empty when the sub-mesh is untextured or has no UVs. */
    };

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
     * @brief Create the GPU resources of the MeshMaterial mode: per-material uniform buffers, per-sub-mesh bindings
     *        and pipeline.
     * @param batch Receives the uniform uploads, and the UV and texture uploads when the shared resources are missing.
     * @note Called lazily, only while MeshMaterial shading is active: GPU UVs and textures are only allocated by the
     *       material modes (MeshMaterial and MeshPBR), which share them. Only the base color textures are uploaded.
     */
    void rebuildMaterialResources(QRhiResourceUpdateBatch* batch);

    /** @brief Release the pipeline, bindings and uniform buffers of the MeshMaterial mode. */
    void releaseMaterialResources();

    /**
     * @brief Create the GPU resources of the MeshPBR mode: frame uniform buffer, per-material uniform buffers,
     *        per-sub-mesh bindings and pipeline.
     * @param batch Receives the uniform uploads, and the UV and texture uploads when the shared resources are missing.
     * @note Called lazily, only while MeshPBR shading is active. Built for any non-empty mesh, textured or not:
     *       untextured slots bind the white texture, so the material factors alone apply.
     */
    void rebuildPbrResources(QRhiResourceUpdateBatch* batch);

    /** @brief Release the pipeline, bindings and uniform buffers of the MeshPBR mode. */
    void releasePbrResources();

    /**
     * @brief Create the resources shared by the MeshMaterial and MeshPBR modes when missing: UV buffer, sampler and
     *        white texture.
     * @param batch Receives the UV and white texture uploads.
     * @note The UV buffer covers every vertex: sub-meshes without UVs get (0, 0), so the buffer is always fully defined.
     */
    void ensureSharedMaterialResources(QRhiResourceUpdateBatch* batch);

    /**
     * @brief Return the GPU texture of a mesh texture, uploading it on first use.
     * @param batch Receives the upload.
     * @param textureIndex Index into _textureImages, or -1.
     * @param srgb True for a base color texture: created as sRGB, so that sampling (filtering and mipmaps included)
     *             returns linear values. False for data textures (metallic-roughness), sampled raw.
     * @return The texture, or the white texture when textureIndex is -1, out of range or the image is empty.
     * @note srgb only matters on the first call for a given index. An index is always used the same way: the loader
     *       caches textures per color space, so a file used both as color and as data gets two distinct indices.
     */
    QRhiTexture* ensureTexture(QRhiResourceUpdateBatch* batch, int textureIndex, bool srgb);

    /**
     * @brief Return the index of the material uniform buffer used by a sub-mesh.
     * @param range The sub-mesh.
     * @return The material index, or _materials.size() (the trailing default buffer) when the sub-mesh has no valid material.
     */
    qsizetype materialBufferIndex(const DrawRange& range) const;

    /** @brief Release the mode-specific resources of both material modes, then the shared ones. */
    void releaseAllMaterialResources();

    /**
     * @brief Build the bindings shared by the material pipelines' layout.
     * @param materialBuffer Per-material uniform buffer (binding 1).
     * @param baseColorTexture Base color texture (binding 2).
     * @return The bindings 0 to 2, common to MeshMaterial and MeshPBR.
     */
    QList<QRhiShaderResourceBinding> commonMaterialBindings(QRhiBuffer* materialBuffer, QRhiTexture* baseColorTexture) const;


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

    /** @name MeshMaterial and MeshPBR modes
     *  CPU data is copied from MeshData on mesh change; GPU resources are created on first use of each mode.
     *  @{ */
    /** @brief Index range and material of each sub-mesh. */
    QVector<DrawRange> _drawRanges;
    /** @brief Copy of the mesh materials. */
    QVector<MeshMaterial> _materials;
    /** @brief Texture images waiting for upload. */
    QVector<std::shared_ptr<const MeshTextureImage>> _textureImages;
    bool _hasBaseColorTexture = false; /**< True when a sub-mesh with UVs has a base color texture, i.e. MeshMaterial can be drawn. */
    bool _materialsDirty = false;      /**< MeshMaterial GPU resources must be (re)built before the next MeshMaterial draw. */
    bool _pbrDirty = false;            /**< MeshPBR GPU resources must be (re)built before the next MeshPBR draw. */
    bool _sharedMaterialDirty = false; /**< The mesh changed: every material GPU resource is stale and must be released. */

    /** @brief GPU texture coordinate buffer, shared by both material modes. */
    std::unique_ptr<QRhiBuffer> _uvBuffer;
    /** @brief Sampler shared by all material textures. */
    std::unique_ptr<QRhiSampler> _materialSampler;
    std::unique_ptr<QRhiTexture> _whiteTexture; /**< Bound for untextured slots. */
    /**
     * @brief GPU textures, parallel to _textureImages; null until a mode needs them.
     * @note Shared by MeshMaterial and MeshPBR: base color textures are sRGB, metallic-roughness textures are raw.
     */
    std::vector<std::unique_ptr<QRhiTexture>> _textures;

    std::vector<std::unique_ptr<QRhiBuffer>> _materialBuffers;              /**< MeshMaterial: one base color + opaque flag UBO per material, plus a trailing default one. */
    std::vector<std::unique_ptr<QRhiShaderResourceBindings>> _materialSrbs; /**< MeshMaterial: one binding set per sub-mesh, parallel to _drawRanges. */
    /** @brief Pipeline for MeshMaterial shading. */
    std::unique_ptr<QRhiGraphicsPipeline> _materialPipeline;

    std::vector<std::unique_ptr<QRhiBuffer>> _pbrMaterialBuffers;      /**< MeshPBR: one base color + factors UBO per material, plus a default one. */
    std::vector<std::unique_ptr<QRhiShaderResourceBindings>> _pbrSrbs; /**< MeshPBR: one binding set per sub-mesh, parallel to _drawRanges. */
    /** @brief MeshPBR per-frame uniform buffer: camera position in world space. */
    std::unique_ptr<QRhiBuffer> _pbrFrameBuffer;
    /** @brief Pipeline for MeshPBR shading. */
    std::unique_ptr<QRhiGraphicsPipeline> _pbrPipeline;
    /** @} */
};
