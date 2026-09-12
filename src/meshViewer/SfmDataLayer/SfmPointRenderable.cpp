#include <SfmDataLayer/SfmPointRenderable.hpp>

#include <QFile>

namespace {

constexpr int kPointSpriteUniformBufferSize = 80;

constexpr QuadVertex k_quadVertices[] = {
  {-1.0f, -1.0f},
  {1.0f, -1.0f},
  {-1.0f, 1.0f},
  {1.0f, 1.0f},
};

constexpr quint16 k_quadIndices[] = {0, 1, 2, 1, 3, 2};

QShader loadSfmPointShader(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(file.readAll());
}

}  // namespace

void SfmPointRenderable::setData(std::vector<SfmDataPointInstance> points, float pointRadius)
{
    _points = std::move(points);
    _instancesDirty = true;

    setPointRadius(pointRadius);
}

void SfmPointRenderable::setPointRadius(float pointRadius)
{
    if (!qFuzzyCompare(_pointRadius, pointRadius))
    {
        _pointRadius = pointRadius;
    }
}

void SfmPointRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
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

void SfmPointRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_geomDirty)
    {
        buildGeometry(batch);
    }

    if (_instancesDirty)
    {
        rebuildInstanceBuffer(batch);
    }

    if (_uniformBuffer)
    {
        UniformData uniforms;
        std::copy_n(state.viewProjection.constData(), 16, uniforms.mvp);
        // Keep point size fixed in screen space but map layer values to useful pixel sizes.
        const float fixedPixelSize = std::max(_pointRadius, 0.0f);
        uniforms.spriteInfo[0] = fixedPixelSize;
        uniforms.spriteInfo[1] = static_cast<float>(state.viewportWidth);
        uniforms.spriteInfo[2] = static_cast<float>(state.viewportHeight);
        uniforms.spriteInfo[3] = 0.0f;
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, sizeof(UniformData), &uniforms);
    }
}

void SfmPointRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (!_pipeline || !_vertexBuffer || !_indexBuffer || _pointRadius == 0.0f)
    {
        return;
    }

    cb->setGraphicsPipeline(_pipeline.get());
    cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
    cb->setShaderResources(_srb.get());

    if (_instanceBuffer && _instanceCount > 0)
    {
        const QRhiCommandBuffer::VertexInput vbs[] = {{_vertexBuffer.get(), 0}, {_instanceBuffer.get(), 0}};
        cb->setVertexInput(0, 2, vbs, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt16);
        cb->drawIndexed(static_cast<quint32>(std::size(k_quadIndices)), _instanceCount);
    }
}

void SfmPointRenderable::buildGeometry(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, static_cast<quint32>(sizeof(k_quadVertices))));
    _vertexBuffer->create();

    _indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, static_cast<quint32>(sizeof(k_quadIndices))));
    _indexBuffer->create();

    batch->uploadStaticBuffer(_vertexBuffer.get(), k_quadVertices);
    batch->uploadStaticBuffer(_indexBuffer.get(), k_quadIndices);

    _geomDirty = false;
}

void SfmPointRenderable::buildPipeline()
{
    _pipeline.reset();
    _srb.reset();
    _uniformBuffer.reset();

    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, kPointSpriteUniformBufferSize));
    _uniformBuffer->create();

    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get())});
    _srb->create();

    QShader vert = loadSfmPointShader(":/shaders/pointSprite.vert.qsb");
    QShader frag = loadSfmPointShader(":/shaders/pointSprite.frag.qsb");

    // Binding 0: per-vertex data (shared unit-quad corner).
    // Binding 1: per-instance data (point position + color).
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings(
      {QRhiVertexInputBinding(sizeof(QuadVertex)), QRhiVertexInputBinding(sizeof(ColoredVertex), QRhiVertexInputBinding::PerInstance)});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
                               QRhiVertexInputAttribute(1, 1, QRhiVertexInputAttribute::Float3, 0),
                               QRhiVertexInputAttribute(1, 2, QRhiVertexInputAttribute::Float3, sizeof(float) * 3)});

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

void SfmPointRenderable::rebuildInstanceBuffer(QRhiResourceUpdateBatch* batch)
{
    _instanceBuffer.reset();
    _instanceCount = 0;
    _instancesDirty = false;

    if (_points.empty())
    {
        return;
    }

    std::vector<ColoredVertex> instances;
    instances.reserve(_points.size());
    for (const SfmDataPointInstance& point : _points)
    {
        instances.push_back({point.position.x(), -point.position.y(), -point.position.z(), point.color.x(), point.color.y(), point.color.z()});
    }

    _instanceCount = static_cast<quint32>(instances.size());
    const quint32 bufSize = _instanceCount * static_cast<quint32>(sizeof(ColoredVertex));

    _instanceBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, bufSize));
    _instanceBuffer->create();
    batch->uploadStaticBuffer(_instanceBuffer.get(), instances.data());
}
