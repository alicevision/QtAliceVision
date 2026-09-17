#include <SphereLayer/SphereRenderable.hpp>
#include <SphereLayer/SphereLayer.hpp>
#include <Core/SceneView.hpp>
#include <QFile>

#include <Geometry/Geometry.hpp>

static QShader loadSphereShader(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(f.readAll());
}

void SphereRenderable::sync(LayerItem* layer, SceneView* view)
{
    if (!view)
    {
        return;
    }

    SphereLayer* sphereLayer = static_cast<SphereLayer*>(layer);
    if (!sphereLayer->positionsDirty())
    {
        return;
    }

    setPositions(sphereLayer->renderPositions());
    sphereLayer->clearPositionsDirty();
}

void SphereRenderable::setPositions(const std::vector<QVector3D>& positions)
{
    _positions = positions;
    _instancesDirty = true;
}

void SphereRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _geomDirty = true;
        _instancesDirty = true;
        _pipeline.reset();
        _srb.reset();
        _uniformBuffer.reset();
        _vertexBuffer.reset();
        _indexBuffer.reset();
        _instanceBuffer.reset();
    }

    if (_pipelineDirty)
    {
        buildPipeline();
    }
}

void SphereRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_geomDirty)
    {
        buildGeometry(batch);
    }

    if (_instancesDirty)
    {
        rebuildInstanceBuffer(batch);
    }

    if (_uniformBuffer && _instanceCount > 0)
    {
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, 64, state.viewProjection.constData());
    }
}

void SphereRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (!_pipeline || !_vertexBuffer || !_indexBuffer || !_instanceBuffer || _instanceCount == 0)
        return;

    cb->setGraphicsPipeline(_pipeline.get());
    cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
    cb->setShaderResources(_srb.get());

    const QRhiCommandBuffer::VertexInput vbs[2] = {{_vertexBuffer.get(), 0}, {_instanceBuffer.get(), 0}};
    cb->setVertexInput(0, 2, vbs, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(_indexCount, _instanceCount);
}

void SphereRenderable::buildGeometry(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset();
    _indexBuffer.reset();
    _indexCount = 0;
    _geomDirty = false;

    QVector<ColoredVertex> verts;
    QVector<quint32> indices;
    buildSphereMesh(verts, indices);

    _indexCount = static_cast<quint32>(indices.size());

    const quint32 vbSize = static_cast<quint32>(verts.size()) * sizeof(ColoredVertex);
    const quint32 ibSize = static_cast<quint32>(indices.size()) * sizeof(quint32);

    _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbSize));
    _vertexBuffer->create();

    _indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibSize));
    _indexBuffer->create();

    batch->uploadStaticBuffer(_vertexBuffer.get(), verts.constData());
    batch->uploadStaticBuffer(_indexBuffer.get(), indices.constData());
}

void SphereRenderable::rebuildInstanceBuffer(QRhiResourceUpdateBatch* batch)
{
    _instanceBuffer.reset();
    _instanceCount = 0;
    _instancesDirty = false;

    if (_positions.empty())
    {
        return;
    }

    _instanceCount = static_cast<quint32>(_positions.size());
    const quint32 bufSize = _instanceCount * static_cast<quint32>(sizeof(QVector3D));

    _instanceBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, bufSize));
    _instanceBuffer->create();
    batch->uploadStaticBuffer(_instanceBuffer.get(), _positions.data());
}

void SphereRenderable::buildPipeline()
{
    _pipeline.reset();
    _srb.reset();
    _uniformBuffer.reset();

    // Uniform buffer: viewProjection (mat4 = 64 bytes)
    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    _uniformBuffer->create();

    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get())});
    _srb->create();

    QShader vert = loadSphereShader(":/shaders/instancedColor.vert.qsb");
    QShader frag = loadSphereShader(":/shaders/simpleColor.frag.qsb");

    // Binding 0: per-vertex data (position + color)
    // Binding 1: per-instance data (world offset, vec3)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings(
      {QRhiVertexInputBinding(sizeof(ColoredVertex)), QRhiVertexInputBinding(sizeof(float) * 3, QRhiVertexInputBinding::PerInstance)});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
                               QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, sizeof(float) * 3),
                               QRhiVertexInputAttribute(1, 2, QRhiVertexInputAttribute::Float3, 0)});

    _pipeline.reset(_rhi->newGraphicsPipeline());
    _pipeline->setShaderStages({
      {QRhiShaderStage::Vertex, vert},
      {QRhiShaderStage::Fragment, frag},
    });
    _pipeline->setVertexInputLayout(inputLayout);
    _pipeline->setShaderResourceBindings(_srb.get());
    _pipeline->setRenderPassDescriptor(_rpDesc);
    _pipeline->setDepthTest(true);
    _pipeline->setDepthWrite(true);
    _pipeline->setCullMode(QRhiGraphicsPipeline::Back);
    _pipeline->create();

    _pipelineDirty = false;
}
