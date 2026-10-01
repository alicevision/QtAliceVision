#include <MeshLayer/MeshRenderable.hpp>
#include <MeshLayer/MeshLayer.hpp>
#include <MeshLayer/MeshData.hpp>

#include <QFile>

static QShader loadMeshShader(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(f.readAll());
}

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
        _meshHasUVs = false;

        quint32 vertexOffset = 0;
        for (const SubMesh& sub : data.subMeshes)
        {
            _drawRanges.append(
              {static_cast<quint32>(_indices.size()), static_cast<quint32>(sub.indices.size()), vertexOffset, sub.materialIndex, sub.uvs});
            _meshHasUVs = _meshHasUVs || !sub.uvs.isEmpty();

            _vertices += sub.vertices;
            for (quint32 idx : sub.indices)
                _indices.append(idx + vertexOffset);
            vertexOffset += sub.vertices.size();
        }

        _materials = data.materials;
        _textureImages = data.textures;

        meshLayer->clearMeshDirty();
        _buffersDirty = true;
        _materialsDirty = true;
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
        releaseMaterialResources();
        _materialsDirty = true;
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

    if (_materialsDirty)
    {
        if (_shadingMode == MeshLayer::MeshMaterial)
        {
            rebuildMaterialResources(batch);
        }
        else if (_materialPipeline)
        {
            // Mesh changed while another mode is active: free the stale GPU resources.
            releaseMaterialResources();
        }
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
}

void MeshRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    const bool drawSolid = (_wireframeMode == MeshLayer::Solid || _wireframeMode == MeshLayer::Overlay);
    const bool drawWire = (_wireframeMode == MeshLayer::Overlay || _wireframeMode == MeshLayer::WireframeOnly);

    if (_depthPipeline && _normalPipeline && _shadedPipeline && _vertexBuffer && _indexBuffer)
    {
        // MeshMaterial falls back to MeshShaded when the mesh has no texture.
        const bool drawMaterial = drawSolid && _shadingMode == MeshLayer::MeshMaterial && _materialPipeline && !_materialsDirty;

        if (drawMaterial)
        {
            cb->setGraphicsPipeline(_materialPipeline.get());
            cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});

            const QRhiCommandBuffer::VertexInput inputs[] = {{_vertexBuffer.get(), 0}, {_uvBuffer.get(), 0}};
            cb->setVertexInput(0, 2, inputs, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);

            for (const DrawRange& range : _drawRanges)
            {
                const bool hasMaterial = range.materialIndex >= 0 && range.materialIndex < _materials.size();
                const size_t srbIndex = hasMaterial ? static_cast<size_t>(range.materialIndex) : _materialSrbs.size() - 1;
                cb->setShaderResources(_materialSrbs[srbIndex].get());
                cb->drawIndexed(range.indexCount, 1, range.firstIndex);
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
          {QRhiShaderStage::Vertex, loadMeshShader(vertPath)},
          {QRhiShaderStage::Fragment, loadMeshShader(fragPath)},
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

    QShader vert = loadMeshShader(":/shaders/meshWire.vert.qsb");
    QShader frag = loadMeshShader(":/shaders/meshWire.frag.qsb");

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
        return;

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
    _textures.clear();
    _whiteTexture.reset();
    _materialSampler.reset();
    _uvBuffer.reset();
}

void MeshRenderable::rebuildMaterialResources(QRhiResourceUpdateBatch* batch)
{
    releaseMaterialResources();
    _materialsDirty = false;

    if (!_meshHasUVs || _vertices.isEmpty() || !_uniformBuffer)
    {
        return;
    }

    // UVs: one buffer index-aligned with the vertex buffer, filled per sub-mesh.
    // Ranges of untextured sub-meshes are left unset: they sample the 1x1 white texture, so any UV is fine.
    _uvBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, static_cast<quint32>(_vertices.size()) * sizeof(UVVertex)));
    _uvBuffer->create();
    for (const DrawRange& range : _drawRanges)
    {
        if (!range.uvs.isEmpty())
        {
            batch->uploadStaticBuffer(_uvBuffer.get(),
                                      range.firstVertex * sizeof(UVVertex),
                                      static_cast<quint32>(range.uvs.size()) * sizeof(UVVertex),
                                      range.uvs.constData());
        }
    }

    // Repeat addressing keeps UDIM-offset UVs (u in [1, 2), ...) sampling their own tile texture.
    _materialSampler.reset(_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Repeat, QRhiSampler::Repeat));
    _materialSampler->create();

    _whiteTexture.reset(_rhi->newTexture(QRhiTexture::RGBA8, QSize(1, 1)));
    _whiteTexture->create();
    const quint32 whitePixel = 0xFFFFFFFF;
    batch->uploadTexture(
      _whiteTexture.get(),
      QRhiTextureUploadDescription(QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(&whitePixel, sizeof(whitePixel)))));

    // Textures: RGBAColor is 4 contiguous bytes per pixel in row-major order, matching RGBA8.
    // The upload description copies the pixels, so the CPU images do not need to outlive the batch.
    for (const std::shared_ptr<const MeshTextureImage>& image : _textureImages)
    {
        const QSize size(image->width(), image->height());
        std::unique_ptr<QRhiTexture> texture(
          _rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips));
        texture->create();

        const quint32 byteCount = static_cast<quint32>(size.width()) * static_cast<quint32>(size.height()) * 4;
        batch->uploadTexture(
          texture.get(),
          QRhiTextureUploadDescription(QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(image->data(), byteCount))));
        batch->generateMips(texture.get());

        _textures.push_back(std::move(texture));
    }

    // One base color buffer and binding set per material, plus a trailing default (white, untextured) one.
    for (qsizetype i = 0; i <= _materials.size(); ++i)
    {
        const bool isDefault = (i == _materials.size());
        const QVector4D color = isDefault ? QVector4D(1.0f, 1.0f, 1.0f, 1.0f) : _materials[i].baseColor;
        const int textureIndex = isDefault ? -1 : _materials[i].textureIndex;
        QRhiTexture* texture = (textureIndex >= 0) ? _textures[static_cast<size_t>(textureIndex)].get() : _whiteTexture.get();

        std::unique_ptr<QRhiBuffer> buffer(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 16));
        buffer->create();
        const float colorData[4] = {color.x(), color.y(), color.z(), color.w()};
        batch->updateDynamicBuffer(buffer.get(), 0, sizeof(colorData), colorData);

        std::unique_ptr<QRhiShaderResourceBindings> srb(_rhi->newShaderResourceBindings());
        srb->setBindings({
          QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, _uniformBuffer.get()),
          QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::FragmentStage, buffer.get()),
          QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, texture, _materialSampler.get()),
        });
        srb->create();

        _materialBuffers.push_back(std::move(buffer));
        _materialSrbs.push_back(std::move(srb));
    }

    // Vertex layout: position (vec3) from the main vertex buffer + UV (vec2) from the UV buffer.
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(Vertex)), QRhiVertexInputBinding(sizeof(UVVertex))});
    inputLayout.setAttributes(
      {QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0), QRhiVertexInputAttribute(1, 1, QRhiVertexInputAttribute::Float2, 0)});

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    // All material SRBs share the same layout, so any of them can define the pipeline's.
    _materialPipeline.reset(_rhi->newGraphicsPipeline());
    _materialPipeline->setShaderStages({
      {QRhiShaderStage::Vertex, loadMeshShader(":/shaders/meshMaterial.vert.qsb")},
      {QRhiShaderStage::Fragment, loadMeshShader(":/shaders/meshMaterial.frag.qsb")},
    });
    _materialPipeline->setVertexInputLayout(inputLayout);
    _materialPipeline->setShaderResourceBindings(_materialSrbs.back().get());
    _materialPipeline->setRenderPassDescriptor(_rpDesc);
    _materialPipeline->setDepthTest(true);
    _materialPipeline->setDepthWrite(true);
    _materialPipeline->setCullMode(QRhiGraphicsPipeline::Back);
    _materialPipeline->setTargetBlends({blend});
    _materialPipeline->create();
}
