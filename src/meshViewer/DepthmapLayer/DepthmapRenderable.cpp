#include <DepthmapLayer/DepthmapRenderable.hpp>
#include <DepthmapLayer/DepthmapLayer.hpp>
#include <DepthmapLayer/DepthmapData.hpp>

#include <QFile>
#include <QMatrix4x4>

static QShader loadDepthmapShader(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(f.readAll());
}

void DepthmapRenderable::sync(LayerItem* layer, SceneView* /*view*/)
{
    DepthmapLayer* depthmapLayer = static_cast<DepthmapLayer*>(layer);

    if (!depthmapLayer->dataDirty())
    {
        return;
    }

    const DepthmapData& data = depthmapLayer->depthmapData();
    _vertices = data.vertices;
    _indices = data.indices;

    depthmapLayer->clearDataDirty();
    _buffersDirty = true;
}

void DepthmapRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _buffersDirty = true;
        _pipeline.reset();
        _srb.reset();
        _uniformBuffer.reset();
        _vertexBuffer.reset();
        _indexBuffer.reset();
    }

    if (_pipelineDirty)
    {
        buildPipeline();
    }
}

void DepthmapRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_buffersDirty)
    {
        rebuildBuffers(batch);
    }

    if (_uniformBuffer)
    {
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, 64, state.viewProjection.constData());
    }
}

void DepthmapRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (!_pipeline || !_vertexBuffer || !_indexBuffer || _indexCount == 0)
    {
        return;
    }

    cb->setGraphicsPipeline(_pipeline.get());
    cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
    cb->setShaderResources(_srb.get());

    const QRhiCommandBuffer::VertexInput vb(_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vb, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(_indexCount);
}

void DepthmapRenderable::rebuildBuffers(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset();
    _indexBuffer.reset();
    _indexCount = 0;
    _buffersDirty = false;

    if (_vertices.isEmpty() || _indices.isEmpty())
    {
        return;
    }

    _indexCount = static_cast<quint32>(_indices.size());

    const quint32 vbSize = static_cast<quint32>(_vertices.size()) * sizeof(ColoredVertex);
    const quint32 ibSize = static_cast<quint32>(_indices.size()) * sizeof(quint32);

    _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbSize));
    _vertexBuffer->create();

    _indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibSize));
    _indexBuffer->create();

    batch->uploadStaticBuffer(_vertexBuffer.get(), _vertices.constData());
    batch->uploadStaticBuffer(_indexBuffer.get(), _indices.constData());
}

void DepthmapRenderable::buildPipeline()
{
    _pipeline.reset();
    _srb.reset();
    _uniformBuffer.reset();

    // Uniform buffer: MVP only (1 x mat4 = 64 bytes)
    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    _uniformBuffer->create();

    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get())});
    _srb->create();

    QShader vert = loadDepthmapShader(":/shaders/simpleColor.vert.qsb");
    QShader frag = loadDepthmapShader(":/shaders/simpleColor.frag.qsb");

    // Vertex layout: position (vec3) + color (vec3)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(ColoredVertex))});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
                               QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, sizeof(float) * 3)});

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
    _pipeline->setCullMode(QRhiGraphicsPipeline::None);
    _pipeline->create();

    _pipelineDirty = false;
}
