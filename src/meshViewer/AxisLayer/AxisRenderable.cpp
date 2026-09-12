#include <AxisLayer/AxisRenderable.hpp>
#include <AxisLayer/AxisGeometry.hpp>
#include <Core/SceneView.hpp>

#include <QFile>
#include <QMatrix4x4>

static QShader loadAxisShader(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(f.readAll());
}

void AxisRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _geomDirty = true;
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

void AxisRenderable::sync(LayerItem* /*layer*/, SceneView* view)
{
    MotionInfo* motionInfo = view->getMotionInfo();

    Eigen::Vector3d center = motionInfo->getCenter();

    _modelMatrix(0, 3) = center(0);
    _modelMatrix(1, 3) = center(1);
    _modelMatrix(2, 3) = center(2);
}

void AxisRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_geomDirty)
    {
        buildGeometry(batch);
    }

    if (_uniformBuffer)
    {
        // Scale by clip-space W so the gizmo keeps a constant screen-space size
        // regardless of how far it is from the camera.
        const QVector4D clipPos = state.viewProjection * QVector4D(_modelMatrix.column(3));
        QMatrix4x4 scaledModel = _modelMatrix;
        scaledModel.scale(clipPos.w() * 0.15f);

        const QMatrix4x4 axisMvp = state.viewProjection * scaledModel;
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, 64, axisMvp.constData());
    }
}

void AxisRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (!_pipeline || !_vertexBuffer || !_indexBuffer)
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

void AxisRenderable::buildGeometry(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset();
    _indexBuffer.reset();
    _indexCount = 0;
    _geomDirty = false;

    QVector<ColoredVertex> verts;
    QVector<quint32> indices;
    buildAxisMesh(verts, indices);

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

void AxisRenderable::buildPipeline()
{
    _pipeline.reset();
    _srb.reset();
    _uniformBuffer.reset();

    // Uniform buffer: MVP only (1 × mat4 = 64 bytes)
    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    _uniformBuffer->create();

    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get())});
    _srb->create();

    QShader vert = loadAxisShader(":/shaders/simpleColor.vert.qsb");
    QShader frag = loadAxisShader(":/shaders/simpleColor.frag.qsb");

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
    _pipeline->setDepthTest(false);
    _pipeline->setDepthWrite(false);
    _pipeline->setCullMode(QRhiGraphicsPipeline::None);
    _pipeline->create();

    _pipelineDirty = false;
}
