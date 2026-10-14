#pragma once

#include <Core/IRenderable.hpp>
#include <MeshLayer/MeshLayer.hpp>
#include <Geometry/vertex.hpp>
#include <MeshLayer/MeshData.hpp>
#include <memory>
#include <vector>

class MeshRenderable : public IRenderable
{
  public:
    void sync(LayerItem* layer, SceneView* view) override;

    void initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc) override;
    void prepare(QRhiResourceUpdateBatch* batch, const SceneState& state) override;
    void render(QRhiCommandBuffer* cb, const SceneState& state) override;

  private:
    void buildPipeline();
    void rebuildBuffers(QRhiResourceUpdateBatch* batch);
    void buildWirePipeline();
    void buildFlatWireVertices(const MeshData& data);
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

    QRhi* _rhi = nullptr;
    QRhiRenderPassDescriptor* _rpDesc = nullptr;

    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _srb;
    std::unique_ptr<QRhiGraphicsPipeline> _depthPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> _normalPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> _shadedPipeline;

    QVector<Vertex> _vertices;
    QVector<quint32> _indices;

    bool _pipelineDirty = true;
    bool _buffersDirty = true;
    QMatrix4x4 _modelMatrix;

    MeshLayer::ShadingMode _shadingMode = MeshLayer::MeshShaded;
    float _meshOpacity = 1.0f;

    MeshLayer::WireframeMode _wireframeMode = MeshLayer::Solid;
    bool _wireframeDirty = true;
    bool _wireBufferDirty = false;

    QVector<WireVertex> _wireVertices;
    std::unique_ptr<QRhiBuffer> _wireVertexBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _wireSrb;
    std::unique_ptr<QRhiGraphicsPipeline> _wireOverlayPipeline;
    std::unique_ptr<QRhiGraphicsPipeline> _wireOnlyPipeline;

    /** @name MeshMaterial mode
     *  CPU data is copied from MeshData on mesh change; GPU resources are created on first use of the mode.
     *  @{ */
    QVector<DrawRange> _drawRanges;
    QVector<MeshMaterial> _materials;
    QVector<std::shared_ptr<const MeshTextureImage>> _textureImages;
    bool _meshHasUVs = false;     /**< True when at least one sub-mesh is textured, i.e. MeshMaterial can be drawn. */
    bool _materialsDirty = false; /**< GPU material resources must be (re)built before the next MeshMaterial draw. */

    std::unique_ptr<QRhiBuffer> _uvBuffer;
    std::unique_ptr<QRhiSampler> _materialSampler;
    std::unique_ptr<QRhiTexture> _whiteTexture; /**< Bound for untextured materials. */
    std::vector<std::unique_ptr<QRhiTexture>> _textures;
    std::vector<std::unique_ptr<QRhiBuffer>> _materialBuffers;              /**< One base color UBO per material, plus a trailing default one. */
    std::vector<std::unique_ptr<QRhiShaderResourceBindings>> _materialSrbs; /**< Parallel to _materialBuffers. */
    std::unique_ptr<QRhiGraphicsPipeline> _materialPipeline;
    /** @} */
};
