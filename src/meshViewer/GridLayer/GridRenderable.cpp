#include <GridLayer/GridRenderable.hpp>
#include <GridLayer/GridLayer.hpp>
#include <Geometry/vertex.hpp>

#include <QFile>
#include <QVector4D>

namespace {

struct GridUniformData
{
    float inverseViewProjection[16];
    float viewportSize[4];
    float spacingAndWidth[4];
    float minorGridParams[4];
};

constexpr QuadVertex k_quadVertices[] = {
  {-1.0f, -1.0f},
  {1.0f, -1.0f},
  {-1.0f, 1.0f},
  {1.0f, 1.0f},
};

constexpr quint16 k_quadIndices[] = {0, 1, 2, 1, 3, 2};

QShader loadShader(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(file.readAll());
}

}  // namespace

void GridRenderable::sync(LayerItem* layer, SceneView* /*view*/)
{
    GridLayer* gridLayer = static_cast<GridLayer*>(layer);
    _majorSpacing = gridLayer->majorSpacing();
    _minorSpacing = gridLayer->minorSpacing();
    _majorLineWidth = gridLayer->majorLineWidth();
    _minorLineWidth = gridLayer->minorLineWidth();
    _minorOpacity = gridLayer->minorOpacity();
    _minorFadeStartPixels = gridLayer->minorFadeStartPixels();
    _minorFadeEndPixels = gridLayer->minorFadeEndPixels();
}

void GridRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _geomDirty = true;
        _pipeline.reset();
        _uniformBuffer.reset();
        _srb.reset();
        _vertexBuffer.reset();
        _indexBuffer.reset();
    }

    if (_pipelineDirty)
    {
        buildPipeline();
    }
}

void GridRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_geomDirty)
    {
        buildGeometry(batch);
    }

    if (_uniformBuffer)
    {
        GridUniformData uboData{};
        const QMatrix4x4 inverseViewProjection = state.viewProjection.inverted();
        std::copy_n(inverseViewProjection.constData(), 16, uboData.inverseViewProjection);
        uboData.viewportSize[0] = static_cast<float>(state.viewportWidth);
        uboData.viewportSize[1] = static_cast<float>(state.viewportHeight);
        uboData.spacingAndWidth[0] = _majorSpacing;
        uboData.spacingAndWidth[1] = _minorSpacing;
        uboData.spacingAndWidth[2] = _majorLineWidth;
        uboData.spacingAndWidth[3] = _minorLineWidth;
        uboData.minorGridParams[0] = _minorOpacity;
        uboData.minorGridParams[1] = _minorFadeStartPixels;
        uboData.minorGridParams[2] = _minorFadeEndPixels;
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, sizeof(GridUniformData), &uboData);
    }
}

void GridRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (!_pipeline || !_vertexBuffer || !_indexBuffer)
    {
        return;
    }

    cb->setGraphicsPipeline(_pipeline.get());
    cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
    cb->setShaderResources(_srb.get());

    const QRhiCommandBuffer::VertexInput vb(_vertexBuffer.get(), 0);
    cb->setVertexInput(0, 1, &vb, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(static_cast<quint32>(std::size(k_quadIndices)));
}

void GridRenderable::buildPipeline()
{
    _pipeline.reset();
    _uniformBuffer.reset();
    _srb.reset();

    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(GridUniformData)));
    _uniformBuffer->create();

    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::FragmentStage, _uniformBuffer.get())});
    _srb->create();

    const QShader vert = loadShader(QStringLiteral(":/shaders/gridLayer.vert.qsb"));
    const QShader frag = loadShader(QStringLiteral(":/shaders/gridLayer.frag.qsb"));

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(QuadVertex))});
    inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float2, 0),
    });

    _pipeline.reset(_rhi->newGraphicsPipeline());
    _pipeline->setShaderStages({
      {QRhiShaderStage::Vertex, vert},
      {QRhiShaderStage::Fragment, frag},
    });
    _pipeline->setVertexInputLayout(inputLayout);
    _pipeline->setShaderResourceBindings(_srb.get());
    _pipeline->setRenderPassDescriptor(_rpDesc);
    _pipeline->setDepthTest(false);
    _pipeline->setDepthWrite(false);
    _pipeline->setCullMode(QRhiGraphicsPipeline::None);
    _pipeline->create();

    _pipelineDirty = false;
}

void GridRenderable::buildGeometry(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, static_cast<quint32>(sizeof(k_quadVertices))));
    _vertexBuffer->create();

    _indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, static_cast<quint32>(sizeof(k_quadIndices))));
    _indexBuffer->create();

    batch->uploadStaticBuffer(_vertexBuffer.get(), k_quadVertices);
    batch->uploadStaticBuffer(_indexBuffer.get(), k_quadIndices);

    _geomDirty = false;
}
