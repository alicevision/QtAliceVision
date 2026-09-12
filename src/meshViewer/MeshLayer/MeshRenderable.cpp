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

        quint32 vertexOffset = 0;
        for (const SubMesh& sub : data.subMeshes)
        {
            _vertices += sub.vertices;
            for (quint32 idx : sub.indices)
                _indices.append(idx + vertexOffset);
            vertexOffset += sub.vertices.size();
        }

        meshLayer->clearMeshDirty();
        _buffersDirty = true;
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
    }

    if (_pipelineDirty)
    {
        buildPipeline();  // must come first — creates _uniformBuffer used by wire SRB
    }

    if (_wireframeDirty)
    {
        if (_wireframeMode != MeshLayer::Solid)
            buildWirePipeline();
        else
        {
            _wireOverlayPipeline.reset();
            _wireOnlyPipeline.reset();
            _wireSrb.reset();
        }
        _wireframeDirty = false;
    }
}

void MeshRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_buffersDirty)
    {
        rebuildBuffers(batch);
    }

    if (_wireBufferDirty)
    {
        rebuildWireBuffer(batch);
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

    if (drawSolid && _normalPipeline && _shadedPipeline && _vertexBuffer && _indexBuffer)
    {
        QRhiGraphicsPipeline* solidPipeline = (_shadingMode == MeshLayer::MeshShaded) ? _shadedPipeline.get() : _normalPipeline.get();

        cb->setGraphicsPipeline(solidPipeline);
        cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
        cb->setShaderResources(_srb.get());

        const QRhiCommandBuffer::VertexInput vb(_vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &vb, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(static_cast<quint32>(_indices.size()));
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

    auto makeSolidPipeline = [&](const QString& vertPath, const QString& fragPath) {
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
        p->setTargetBlends({solidBlend});
        p->create();
        return p;
    };

    _normalPipeline = makeSolidPipeline(":/shaders/meshNormal.vert.qsb", ":/shaders/meshNormal.frag.qsb");
    _shadedPipeline = makeSolidPipeline(":/shaders/meshShaded.vert.qsb", ":/shaders/meshShaded.frag.qsb");

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
    _wireOverlayPipeline->setDepthBias(-2);
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
