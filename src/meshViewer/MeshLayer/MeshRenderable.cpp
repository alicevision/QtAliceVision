#include <MeshLayer/MeshRenderable.hpp>
#include <MeshLayer/MeshLayer.hpp>
#include <MeshLayer/MeshData.hpp>

#include <Core/ShaderLoader.hpp>

#include <algorithm>

void MeshRenderable::sync(LayerItem* layer, SceneView* /*view*/)
{
    MeshLayer* meshLayer = static_cast<MeshLayer*>(layer);

    const bool meshChanged = meshLayer->meshDirty();
    const bool modeChanged = meshLayer->wireframeDirty();
    const bool shadingChanged = meshLayer->shadingDirty();
    const bool opacityChanged = meshLayer->opacityDirty();

    if (!meshChanged && !modeChanged && !shadingChanged && !opacityChanged)
    {
        return;
    }

    if (opacityChanged)
    {
        _meshOpacity = meshLayer->opacity();
        meshLayer->clearOpacityDirty();
    }

    if (shadingChanged)
    {
        _shadingMode = meshLayer->shadingMode();
        meshLayer->clearShadingDirty();
    }

    if (modeChanged)
    {
        _wireframeMode = meshLayer->wireframeMode();
        meshLayer->clearWireframeDirty();
        _wireframeDirty = true;
    }

    if (meshChanged)
    {
        const MeshData& data = meshLayer->meshData();

        _vertices.clear();
        _indices.clear();
        _drawRanges.clear();
        _hasBaseColorTexture = false;

        quint32 vertexOffset = 0;
        for (const SubMesh& sub : data.subMeshes)
        {
            _drawRanges.append(
              {static_cast<quint32>(_indices.size()), static_cast<quint32>(sub.indices.size()), vertexOffset, sub.materialIndex, sub.uvs});

            // MeshMaterial only samples the base color texture: a sub-mesh with only a metallic-roughness texture
            // must not enable it, so that the mode keeps falling back to MeshShaded exactly as before.
            const bool hasBaseColor =
              !sub.uvs.isEmpty() && sub.materialIndex >= 0 && sub.materialIndex < data.materials.size() && data.materials[sub.materialIndex].textureIndex >= 0;
            _hasBaseColorTexture = _hasBaseColorTexture || hasBaseColor;

            _vertices += sub.vertices;
            for (quint32 idx : sub.indices)
            {
                _indices.append(idx + vertexOffset);
            }
            vertexOffset += sub.vertices.size();
        }

        _materials = data.materials;
        _textureImages = data.textures;

        meshLayer->clearMeshDirty();
        _buffersDirty = true;
        _materialsDirty = true;
        _pbrDirty = true;
        _sharedMaterialDirty = true;
    }

    // Build (or free) the flat wire vertex buffer whenever the mesh or mode changes.
    if (_wireframeMode != MeshLayer::Solid && (meshChanged || modeChanged))
    {
        buildFlatWireVertices(meshLayer->meshData());
        _wireBufferDirty = true;
    }
    else if (_wireframeMode == MeshLayer::Solid && modeChanged)
    {
        _wireVertices.clear();
        _wireBufferDirty = true;  // signals rebuildWireBuffer to free the GPU buffer
    }
}

void MeshRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _buffersDirty = true;
        _wireframeDirty = true;
        _wireBufferDirty = true;
        _normalPipeline.reset();
        _shadedPipeline.reset();
        _srb.reset();
        _uniformBuffer.reset();
        _vertexBuffer.reset();
        _indexBuffer.reset();
        _wireOverlayPipeline.reset();
        _wireOnlyPipeline.reset();
        _wireSrb.reset();
        _wireVertexBuffer.reset();
        releaseAllMaterialResources();
        _materialsDirty = true;
        _pbrDirty = true;
    }

    if (_pipelineDirty)
    {
        buildPipeline();  // must come first — creates _uniformBuffer used by wire SRB
    }
}

void MeshRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    /**
     * @note The wire pipeline is (re)built here rather than in initialize(): QQuickRhiItemRenderer::initialize()
     *       only runs on first use or when the render target changes, so a wireframe mode switch would
     *       otherwise not be visible until an unrelated event forced a re-initialization.
     */
    if (_wireframeDirty && _uniformBuffer)
    {
        if (_wireframeMode != MeshLayer::Solid)
        {
            buildWirePipeline();
        }
        else
        {
            _wireOverlayPipeline.reset();
            _wireOnlyPipeline.reset();
            _wireSrb.reset();
        }
        _wireframeDirty = false;
    }

    if (_buffersDirty)
    {
        rebuildBuffers(batch);
    }

    if (_wireBufferDirty)
    {
        rebuildWireBuffer(batch);
    }

    if (_sharedMaterialDirty)
    {
        // Mesh changed: free the stale GPU resources of both material modes, whichever mode is active.
        releaseAllMaterialResources();
        _sharedMaterialDirty = false;
    }

    if (_materialsDirty && _shadingMode == MeshLayer::MeshMaterial)
    {
        rebuildMaterialResources(batch);
    }

    if (_pbrDirty && _shadingMode == MeshLayer::MeshPBR)
    {
        rebuildPbrResources(batch);
    }

    if (_uniformBuffer)
    {
        QMatrix4x4 mvp = state.viewProjection * _modelMatrix;
        float uboData[36] = {0.0f};
        memcpy(uboData, mvp.constData(), 64);
        memcpy(uboData + 16, state.normalMatrix.constData(), 64);
        uboData[32] = _meshOpacity;
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, 144, uboData);
    }

    // MeshPBR only: the camera position, in a buffer of its own so the shared uniform buffer is unchanged.
    if (_pbrFrameBuffer)
    {
        const float frameData[4] = {state.cameraWorldPosition.x(), state.cameraWorldPosition.y(), state.cameraWorldPosition.z(), 1.0f};
        batch->updateDynamicBuffer(_pbrFrameBuffer.get(), 0, sizeof(frameData), frameData);
    }
}

void MeshRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    const bool drawSolid = (_wireframeMode == MeshLayer::Solid || _wireframeMode == MeshLayer::Overlay);
    const bool drawWire = (_wireframeMode == MeshLayer::Overlay || _wireframeMode == MeshLayer::WireframeOnly);

    if (_depthPipeline && _normalPipeline && _shadedPipeline && _vertexBuffer && _indexBuffer)
    {
        // MeshMaterial falls back to MeshShaded when the mesh has no base color texture; MeshPBR draws any non-empty mesh.
        const bool drawMaterial = drawSolid && _shadingMode == MeshLayer::MeshMaterial && _materialPipeline && !_materialsDirty;
        const bool drawPbr = drawSolid && _shadingMode == MeshLayer::MeshPBR && _pbrPipeline && !_pbrDirty;

        if (drawMaterial || drawPbr)
        {
            QRhiGraphicsPipeline* pipeline = drawPbr ? _pbrPipeline.get() : _materialPipeline.get();
            const std::vector<std::unique_ptr<QRhiShaderResourceBindings>>& srbs = drawPbr ? _pbrSrbs : _materialSrbs;

            cb->setGraphicsPipeline(pipeline);
            cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});

            // MeshPBR reads the normal from the main vertex buffer too (attribute 2 of binding 0).
            const QRhiCommandBuffer::VertexInput inputs[] = {{_vertexBuffer.get(), 0}, {_uvBuffer.get(), 0}};
            cb->setVertexInput(0, 2, inputs, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);

            // One binding set per sub-mesh, built in the same order as _drawRanges.
            for (qsizetype i = 0; i < _drawRanges.size(); ++i)
            {
                cb->setShaderResources(srbs[static_cast<size_t>(i)].get());
                cb->drawIndexed(_drawRanges[i].indexCount, 1, _drawRanges[i].firstIndex);
            }
        }
        else
        {
            QRhiGraphicsPipeline* solidPipeline;
            if (drawSolid)
            {
                solidPipeline = (_shadingMode == MeshLayer::MeshNormal) ? _normalPipeline.get() : _shadedPipeline.get();
            }
            else
            {
                solidPipeline = _depthPipeline.get();
            }

            cb->setGraphicsPipeline(solidPipeline);
            cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
            cb->setShaderResources(_srb.get());

            const QRhiCommandBuffer::VertexInput vb(_vertexBuffer.get(), 0);
            cb->setVertexInput(0, 1, &vb, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
            cb->drawIndexed(static_cast<quint32>(_indices.size()));
        }
    }

    if (drawWire && _wireVertexBuffer)
    {
        QRhiGraphicsPipeline* wirePipeline = (_wireframeMode == MeshLayer::Overlay) ? _wireOverlayPipeline.get() : _wireOnlyPipeline.get();

        if (wirePipeline && _wireSrb)
        {
            cb->setGraphicsPipeline(wirePipeline);
            cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
            cb->setShaderResources(_wireSrb.get());

            const QRhiCommandBuffer::VertexInput vb(_wireVertexBuffer.get(), 0);
            cb->setVertexInput(0, 1, &vb);
            cb->draw(static_cast<quint32>(_wireVertices.size()));
        }
    }
}

void MeshRenderable::rebuildBuffers(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset();
    _indexBuffer.reset();
    _buffersDirty = false;

    if (_vertices.isEmpty() || _indices.isEmpty())
    {
        return;
    }

    const quint32 vbSize = static_cast<quint32>(_vertices.size()) * sizeof(Vertex);
    const quint32 ibSize = static_cast<quint32>(_indices.size()) * sizeof(quint32);

    _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbSize));
    _vertexBuffer->create();

    _indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibSize));
    _indexBuffer->create();

    batch->uploadStaticBuffer(_vertexBuffer.get(), _vertices.constData());
    batch->uploadStaticBuffer(_indexBuffer.get(), _indices.constData());
}

void MeshRenderable::buildPipeline()
{
    _normalPipeline.reset();
    _shadedPipeline.reset();
    _srb.reset();
    _uniformBuffer.reset();

    // Uniform buffer: MVP + normalMatrix + opacity (std140 aligned to 144 bytes)
    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 144));
    _uniformBuffer->create();

    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
      0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, _uniformBuffer.get())});
    _srb->create();

    // Vertex layout: position (vec3) + normal (vec3)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(Vertex))});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
                               QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, sizeof(float) * 3)});

    // Enable alpha blending so fragment alpha driven by layer opacity
    // contributes to the final color.
    QRhiGraphicsPipeline::TargetBlend solidBlend;
    solidBlend.enable = true;
    solidBlend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    solidBlend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    solidBlend.srcAlpha = QRhiGraphicsPipeline::One;
    solidBlend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    QRhiGraphicsPipeline::TargetBlend noColor;
    noColor.colorWrite = {};

    auto makeSolidPipeline = [&](const QString& vertPath, const QString& fragPath, bool blended) {
        std::unique_ptr<QRhiGraphicsPipeline> p(_rhi->newGraphicsPipeline());
        p->setShaderStages({
          {QRhiShaderStage::Vertex, loadShader(vertPath)},
          {QRhiShaderStage::Fragment, loadShader(fragPath)},
        });
        p->setVertexInputLayout(inputLayout);
        p->setShaderResourceBindings(_srb.get());
        p->setRenderPassDescriptor(_rpDesc);
        p->setDepthTest(true);
        p->setDepthWrite(true);
        p->setCullMode(QRhiGraphicsPipeline::Back);
        if (blended)
        {
            p->setTargetBlends({solidBlend});
        }
        else
        {
            p->setDepthBias(1);
            p->setSlopeScaledDepthBias(1.0f);
            p->setTargetBlends({noColor});
        }
        p->create();
        return p;
    };

    _normalPipeline = makeSolidPipeline(":/shaders/meshNormal.vert.qsb", ":/shaders/meshNormal.frag.qsb", true);
    _shadedPipeline = makeSolidPipeline(":/shaders/meshShaded.vert.qsb", ":/shaders/meshShaded.frag.qsb", true);
    _depthPipeline = makeSolidPipeline(":/shaders/meshShaded.vert.qsb", ":/shaders/meshShaded.frag.qsb", false);

    _pipelineDirty = false;
}

void MeshRenderable::buildWirePipeline()
{
    _wireOverlayPipeline.reset();
    _wireOnlyPipeline.reset();
    _wireSrb.reset();

    // Reuse the same uniform buffer as the solid pipeline.
    _wireSrb.reset(_rhi->newShaderResourceBindings());
    _wireSrb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
      0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, _uniformBuffer.get())});
    _wireSrb->create();

    QShader vert = loadShader(":/shaders/meshWire.vert.qsb");
    QShader frag = loadShader(":/shaders/meshWire.frag.qsb");

    // Vertex layout: position (vec3) + barycentric (vec3)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(WireVertex))});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
                               QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, sizeof(float) * 3)});

    // Alpha blending for AA edge transitions.
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    // Common pipeline setup shared between both wire pipelines.
    auto configure = [&](QRhiGraphicsPipeline* p) {
        p->setShaderStages({
          {QRhiShaderStage::Vertex, vert},
          {QRhiShaderStage::Fragment, frag},
        });
        p->setVertexInputLayout(inputLayout);
        p->setShaderResourceBindings(_wireSrb.get());
        p->setRenderPassDescriptor(_rpDesc);
        p->setTopology(QRhiGraphicsPipeline::Triangles);
        p->setCullMode(QRhiGraphicsPipeline::None);
        p->setDepthTest(true);
        p->setTargetBlends({blend});
    };

    // Overlay: draw on top of the solid mesh.
    // Depth write is off (solid already wrote depth); a small depth bias
    // pulls edges in front of the coincident solid surface to avoid z-fighting.
    _wireOverlayPipeline.reset(_rhi->newGraphicsPipeline());
    configure(_wireOverlayPipeline.get());
    _wireOverlayPipeline->setDepthWrite(false);
    _wireOverlayPipeline->setDepthBias(-1);
    _wireOverlayPipeline->setSlopeScaledDepthBias(-1.0f);
    _wireOverlayPipeline->create();

    // WireframeOnly: no solid mesh drawn, so depth write must be on so that
    // front edges correctly occlude back edges.
    _wireOnlyPipeline.reset(_rhi->newGraphicsPipeline());
    configure(_wireOnlyPipeline.get());
    _wireOnlyPipeline->setDepthWrite(true);
    _wireOnlyPipeline->create();
}

void MeshRenderable::buildFlatWireVertices(const MeshData& data)
{
    _wireVertices.clear();

    for (const SubMesh& sub : data.subMeshes)
    {
        const int triCount = sub.indices.size() / 3;
        _wireVertices.reserve(_wireVertices.size() + triCount * 3);

        for (int i = 0; i < triCount; ++i)
        {
            const Vertex& v0 = sub.vertices[sub.indices[i * 3 + 0]];
            const Vertex& v1 = sub.vertices[sub.indices[i * 3 + 1]];
            const Vertex& v2 = sub.vertices[sub.indices[i * 3 + 2]];
            _wireVertices.append({v0.x, v0.y, v0.z, 1.f, 0.f, 0.f});
            _wireVertices.append({v1.x, v1.y, v1.z, 0.f, 1.f, 0.f});
            _wireVertices.append({v2.x, v2.y, v2.z, 0.f, 0.f, 1.f});
        }
    }
}

void MeshRenderable::rebuildWireBuffer(QRhiResourceUpdateBatch* batch)
{
    _wireVertexBuffer.reset();
    _wireBufferDirty = false;

    if (_wireVertices.isEmpty())
    {
        return;
    }

    const quint32 vbSize = static_cast<quint32>(_wireVertices.size()) * sizeof(WireVertex);
    _wireVertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbSize));
    _wireVertexBuffer->create();
    batch->uploadStaticBuffer(_wireVertexBuffer.get(), _wireVertices.constData());
}

void MeshRenderable::releaseMaterialResources()
{
    _materialPipeline.reset();
    _materialSrbs.clear();
    _materialBuffers.clear();
}

void MeshRenderable::releasePbrResources()
{
    _pbrPipeline.reset();
    _pbrSrbs.clear();
    _pbrMaterialBuffers.clear();
    _pbrFrameBuffer.reset();
}

void MeshRenderable::releaseAllMaterialResources()
{
    // Mode-specific bindings reference the shared textures and sampler: release them first.
    releaseMaterialResources();
    releasePbrResources();
    _textures.clear();
    _whiteTexture.reset();
    _materialSampler.reset();
    _uvBuffer.reset();
}

void MeshRenderable::ensureSharedMaterialResources(QRhiResourceUpdateBatch* batch)
{
    if (!_uvBuffer)
    {
        // UVs: one buffer index-aligned with the vertex buffer. Sub-meshes without UVs get (0, 0): they bind the
        // 1x1 white texture only, so any UV is fine, but the whole buffer stays defined.
        QVector<UVVertex> uvs(_vertices.size(), UVVertex{0.0f, 0.0f});
        for (const DrawRange& range : _drawRanges)
        {
            std::copy(range.uvs.cbegin(), range.uvs.cend(), uvs.begin() + range.firstVertex);
        }

        _uvBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, static_cast<quint32>(uvs.size()) * sizeof(UVVertex)));
        _uvBuffer->create();
        batch->uploadStaticBuffer(_uvBuffer.get(), uvs.constData());
    }

    if (!_materialSampler)
    {
        // Repeat addressing keeps UDIM-offset UVs (u in [1, 2), ...) sampling their own tile texture.
        _materialSampler.reset(
          _rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Repeat, QRhiSampler::Repeat));
        _materialSampler->create();
    }

    if (!_whiteTexture)
    {
        _whiteTexture.reset(_rhi->newTexture(QRhiTexture::RGBA8, QSize(1, 1)));
        _whiteTexture->create();
        const quint32 whitePixel = 0xFFFFFFFF;
        batch->uploadTexture(
          _whiteTexture.get(),
          QRhiTextureUploadDescription(QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(&whitePixel, sizeof(whitePixel)))));
    }

    if (_textures.size() != static_cast<size_t>(_textureImages.size()))
    {
        _textures.clear();
        _textures.resize(static_cast<size_t>(_textureImages.size()));
    }
}

QRhiTexture* MeshRenderable::ensureTexture(QRhiResourceUpdateBatch* batch, int textureIndex, bool srgb)
{
    if (textureIndex < 0 || static_cast<size_t>(textureIndex) >= _textures.size())
    {
        return _whiteTexture.get();
    }

    const std::shared_ptr<const MeshTextureImage>& image = _textureImages[textureIndex];
    if (!image || image->width() <= 0 || image->height() <= 0)
    {
        return _whiteTexture.get();
    }

    std::unique_ptr<QRhiTexture>& texture = _textures[static_cast<size_t>(textureIndex)];
    if (!texture)
    {
        // RGBAColor is 4 contiguous bytes per pixel in row-major order, matching RGBA8.
        // The upload description copies the pixels, so the CPU image does not need to outlive the batch.
        const QSize size(image->width(), image->height());
        QRhiTexture::Flags flags = QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;
        if (srgb)
        {
            flags |= QRhiTexture::sRGB;
        }
        texture.reset(_rhi->newTexture(QRhiTexture::RGBA8, size, 1, flags));
        texture->create();

        const quint32 byteCount = static_cast<quint32>(size.width()) * static_cast<quint32>(size.height()) * 4;
        batch->uploadTexture(
          texture.get(),
          QRhiTextureUploadDescription(QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(image->data(), byteCount))));
        batch->generateMips(texture.get());
    }

    return texture.get();
}

qsizetype MeshRenderable::materialBufferIndex(const DrawRange& range) const
{
    const bool hasMaterial = range.materialIndex >= 0 && range.materialIndex < _materials.size();
    return hasMaterial ? range.materialIndex : _materials.size();
}

QList<QRhiShaderResourceBinding> MeshRenderable::commonMaterialBindings(QRhiBuffer* materialBuffer, QRhiTexture* baseColorTexture) const
{
    return {
      QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, _uniformBuffer.get()),
      QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::FragmentStage, materialBuffer),
      QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, baseColorTexture, _materialSampler.get()),
    };
}

namespace {

/**
 * @brief Alpha blending used by the material pipelines, so that the layer opacity applies.
 * @return The blend state.
 */
QRhiGraphicsPipeline::TargetBlend materialBlend()
{
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    return blend;
}

}  // namespace

void MeshRenderable::rebuildMaterialResources(QRhiResourceUpdateBatch* batch)
{
    releaseMaterialResources();
    _materialsDirty = false;

    if (!_hasBaseColorTexture || _vertices.isEmpty() || !_uniformBuffer)
    {
        return;
    }

    ensureSharedMaterialResources(batch);

    // One base color + opaque flag buffer per material, plus a trailing default (white) one.
    for (qsizetype i = 0; i <= _materials.size(); ++i)
    {
        const MeshMaterial material = (i == _materials.size()) ? MeshMaterial() : _materials[i];

        std::unique_ptr<QRhiBuffer> buffer(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
        buffer->create();
        const float materialData[8] = {material.baseColor.x(),
                                       material.baseColor.y(),
                                       material.baseColor.z(),
                                       material.baseColor.w(),
                                       material.opaque ? 1.0f : 0.0f,
                                       0.0f,
                                       0.0f,
                                       0.0f};
        batch->updateDynamicBuffer(buffer.get(), 0, sizeof(materialData), materialData);
        _materialBuffers.push_back(std::move(buffer));
    }

    // One binding set per sub-mesh: a sub-mesh without UVs binds the white texture rather than sampling its
    // material texture at meaningless coordinates, and keeps its base color.
    for (const DrawRange& range : _drawRanges)
    {
        const qsizetype bufferIndex = materialBufferIndex(range);
        const int textureIndex = (range.uvs.isEmpty() || bufferIndex == _materials.size()) ? -1 : _materials[bufferIndex].textureIndex;
        // Same sRGB texture as MeshPBR: the shader re-encodes to sRGB, so one GPU copy serves both modes.
        QRhiTexture* texture = ensureTexture(batch, textureIndex, true);

        const QList<QRhiShaderResourceBinding> bindings = commonMaterialBindings(_materialBuffers[static_cast<size_t>(bufferIndex)].get(), texture);
        std::unique_ptr<QRhiShaderResourceBindings> srb(_rhi->newShaderResourceBindings());
        srb->setBindings(bindings.cbegin(), bindings.cend());
        srb->create();
        _materialSrbs.push_back(std::move(srb));
    }

    // Vertex layout: position (vec3) from the main vertex buffer + UV (vec2) from the UV buffer.
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(Vertex)), QRhiVertexInputBinding(sizeof(UVVertex))});
    inputLayout.setAttributes(
      {QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0), QRhiVertexInputAttribute(1, 1, QRhiVertexInputAttribute::Float2, 0)});

    // All material SRBs share the same layout, so any of them can define the pipeline's.
    _materialPipeline.reset(_rhi->newGraphicsPipeline());
    _materialPipeline->setShaderStages({
      {QRhiShaderStage::Vertex, loadShader(":/shaders/meshMaterial.vert.qsb")},
      {QRhiShaderStage::Fragment, loadShader(":/shaders/meshMaterial.frag.qsb")},
    });
    _materialPipeline->setVertexInputLayout(inputLayout);
    _materialPipeline->setShaderResourceBindings(_materialSrbs.back().get());
    _materialPipeline->setRenderPassDescriptor(_rpDesc);
    _materialPipeline->setDepthTest(true);
    _materialPipeline->setDepthWrite(true);
    _materialPipeline->setCullMode(QRhiGraphicsPipeline::Back);
    _materialPipeline->setTargetBlends({materialBlend()});
    _materialPipeline->create();
}

void MeshRenderable::rebuildPbrResources(QRhiResourceUpdateBatch* batch)
{
    releasePbrResources();
    _pbrDirty = false;

    if (_vertices.isEmpty() || _drawRanges.isEmpty() || !_uniformBuffer)
    {
        return;
    }

    ensureSharedMaterialResources(batch);

    // Camera position, updated every frame in prepare().
    _pbrFrameBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 16));
    _pbrFrameBuffer->create();

    // One base color + factors buffer per material, plus a trailing default one
    // (white, dielectric and fully rough: plain diffuse).
    for (qsizetype i = 0; i <= _materials.size(); ++i)
    {
        const MeshMaterial material = (i == _materials.size()) ? MeshMaterial() : _materials[i];

        std::unique_ptr<QRhiBuffer> buffer(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 32));
        buffer->create();
        const float materialData[8] = {material.baseColor.x(),
                                       material.baseColor.y(),
                                       material.baseColor.z(),
                                       material.baseColor.w(),
                                       material.metallicFactor,
                                       material.roughnessFactor,
                                       material.opaque ? 1.0f : 0.0f,
                                       0.0f};
        batch->updateDynamicBuffer(buffer.get(), 0, sizeof(materialData), materialData);
        _pbrMaterialBuffers.push_back(std::move(buffer));
    }

    // One binding set per sub-mesh. Untextured slots, and every slot of a sub-mesh without UVs, bind the white
    // texture: the material factors alone then apply.
    for (const DrawRange& range : _drawRanges)
    {
        const qsizetype bufferIndex = materialBufferIndex(range);
        const bool sampleTextures = !range.uvs.isEmpty() && bufferIndex < _materials.size();
        const int baseColorIndex = sampleTextures ? _materials[bufferIndex].textureIndex : -1;
        const int metallicRoughnessIndex = sampleTextures ? _materials[bufferIndex].metallicRoughnessTextureIndex : -1;
        // sRGB texture (shared with MeshMaterial): the shader receives linear base colors, filtered in linear space.
        QRhiTexture* baseColorTexture = ensureTexture(batch, baseColorIndex, true);
        QRhiTexture* metallicRoughnessTexture = ensureTexture(batch, metallicRoughnessIndex, false);

        QList<QRhiShaderResourceBinding> bindings =
          commonMaterialBindings(_pbrMaterialBuffers[static_cast<size_t>(bufferIndex)].get(), baseColorTexture);
        bindings.append(
          QRhiShaderResourceBinding::sampledTexture(3, QRhiShaderResourceBinding::FragmentStage, metallicRoughnessTexture, _materialSampler.get()));
        bindings.append(QRhiShaderResourceBinding::uniformBuffer(4, QRhiShaderResourceBinding::FragmentStage, _pbrFrameBuffer.get()));

        std::unique_ptr<QRhiShaderResourceBindings> srb(_rhi->newShaderResourceBindings());
        srb->setBindings(bindings.cbegin(), bindings.cend());
        srb->create();
        _pbrSrbs.push_back(std::move(srb));
    }

    // Vertex layout: position (vec3) and normal (vec3) from the main vertex buffer + UV (vec2) from the UV buffer.
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(Vertex)), QRhiVertexInputBinding(sizeof(UVVertex))});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
                               QRhiVertexInputAttribute(1, 1, QRhiVertexInputAttribute::Float2, 0),
                               QRhiVertexInputAttribute(0, 2, QRhiVertexInputAttribute::Float3, sizeof(float) * 3)});

    // All PBR SRBs share the same layout, so any of them can define the pipeline's.
    _pbrPipeline.reset(_rhi->newGraphicsPipeline());
    _pbrPipeline->setShaderStages({
      {QRhiShaderStage::Vertex, loadShader(":/shaders/meshPBR.vert.qsb")},
      {QRhiShaderStage::Fragment, loadShader(":/shaders/meshPBR.frag.qsb")},
    });
    _pbrPipeline->setVertexInputLayout(inputLayout);
    _pbrPipeline->setShaderResourceBindings(_pbrSrbs.back().get());
    _pbrPipeline->setRenderPassDescriptor(_rpDesc);
    _pbrPipeline->setDepthTest(true);
    _pbrPipeline->setDepthWrite(true);
    _pbrPipeline->setCullMode(QRhiGraphicsPipeline::Back);
    _pbrPipeline->setTargetBlends({materialBlend()});
    _pbrPipeline->create();
}
