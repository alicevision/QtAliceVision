#include <SfmDataLayer/SfmCameraRenderable.hpp>

#include <Geometry/Geometry.hpp>

#include <QFile>

namespace {

QShader loadSfmCameraShader(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(file.readAll());
}

void appendMesh(QVector<ColoredVertex>& dstVerts,
                QVector<quint32>& dstIndices,
                const QVector<ColoredVertex>& srcVerts,
                const QVector<quint32>& srcIndices,
                const QMatrix4x4& transform)
{
    const quint32 baseIndex = static_cast<quint32>(dstVerts.size());

    for (const ColoredVertex& vert : srcVerts)
    {
        const QVector3D position = transform.map(QVector3D(vert.x, vert.y, vert.z));
        dstVerts.append({position.x(), position.y(), position.z(), vert.r, vert.g, vert.b});
    }

    for (quint32 index : srcIndices)
    {
        dstIndices.append(baseIndex + index);
    }
}

}  // namespace

void SfmCameraRenderable::setData(std::vector<SfmDataCameraInstance> cameras, float cameraScale)
{
    _cameras = std::move(cameras);

    if (!qFuzzyCompare(_cameraScale, cameraScale))
    {
        _cameraScale = cameraScale;
    }

    _geometryDirty = true;
    _selectedGeometryDirty = true;
}

void SfmCameraRenderable::setSelectedCamera(std::optional<size_t> cameraIndex)
{
    if (_selectedCameraIndex == cameraIndex)
    {
        return;
    }

    _selectedCameraIndex = cameraIndex;
    _selectedGeometryDirty = true;
}

void SfmCameraRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _geometryDirty = true;
        _pipeline.reset();
        _srb.reset();
        _uniformBuffer.reset();
        _vertexBuffer.reset();
        _indexBuffer.reset();
        _selectedVertexBuffer.reset();
        _selectedIndexBuffer.reset();
    }

    if (_pipelineDirty)
    {
        buildPipeline();
    }
}

void SfmCameraRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_geometryDirty)
    {
        buildGeometry(batch);
    }

    if (_selectedGeometryDirty)
    {
        buildSelectedGeometry(batch);
    }

    if (_uniformBuffer && _indexCount > 0)
    {
        batch->updateDynamicBuffer(_uniformBuffer.get(), 0, 64, state.viewProjection.constData());
    }
}

void SfmCameraRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (!_pipeline || !_vertexBuffer || !_indexBuffer || _indexCount == 0)
    {
        return;
    }

    cb->setGraphicsPipeline(_pipeline.get());
    cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
    cb->setShaderResources(_srb.get());

    const QRhiCommandBuffer::VertexInput vbs[] = {{_vertexBuffer.get(), 0}};
    cb->setVertexInput(0, 1, vbs, _indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(_indexCount);

    if (_selectedVertexBuffer && _selectedIndexBuffer && _selectedIndexCount > 0)
    {
        const QRhiCommandBuffer::VertexInput selectedVbs[] = {{_selectedVertexBuffer.get(), 0}};
        cb->setVertexInput(0, 1, selectedVbs, _selectedIndexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(_selectedIndexCount);
    }
}

void SfmCameraRenderable::buildPipeline()
{
    _pipeline.reset();
    _srb.reset();
    _uniformBuffer.reset();

    _uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    _uniformBuffer->create();

    _srb.reset(_rhi->newShaderResourceBindings());
    _srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get())});
    _srb->create();

    QShader vert = loadSfmCameraShader(":/shaders/simpleColor.vert.qsb");
    QShader frag = loadSfmCameraShader(":/shaders/simpleColor.frag.qsb");

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
    _pipeline->setTopology(QRhiGraphicsPipeline::Lines);
    _pipeline->setDepthTest(true);
    _pipeline->setDepthWrite(true);
    _pipeline->setCullMode(QRhiGraphicsPipeline::None);
    _pipeline->create();

    _pipelineDirty = false;
}

void SfmCameraRenderable::buildGeometry(QRhiResourceUpdateBatch* batch)
{
    _vertexBuffer.reset();
    _indexBuffer.reset();
    _indexCount = 0;
    _geometryDirty = false;

    if (_cameras.empty() || _cameraScale == 0.0f)
    {
        return;
    }

    QVector<ColoredVertex> cameraVerts;
    QVector<quint32> cameraIndices;

    for (const SfmDataCameraInstance& camera : _cameras)
    {
        QVector<ColoredVertex> localVerts;
        QVector<quint32> localIndices;

        const float scaledSphereRadius = std::max(sphereRadius * _cameraScale, 0.001f);
        const float scaledDepth = std::max(cameraDepth * _cameraScale, 0.0001f);
        const float fovDegrees = camera.intrinsics->getHorizontalFov() * 180.0 / M_PI;
        const float aspectRatio = static_cast<float>(camera.intrinsics->w()) / static_cast<float>(camera.intrinsics->h());

        if (fovDegrees > 180.0f)
        {
            buildSphereWireframeMesh(localVerts, localIndices, scaledSphereRadius, 1.0f, 1.0f, 1.0f, 8, 12);
        }
        else
        {
            buildCameraWireframeMesh(localVerts, localIndices, fovDegrees, scaledDepth, aspectRatio);
        }

        appendMesh(cameraVerts, cameraIndices, localVerts, localIndices, toQMatrix4x4(camera.camera_T_world.inverse()));
    }

    if (cameraVerts.empty() || cameraIndices.empty())
    {
        return;
    }

    _indexCount = static_cast<quint32>(cameraIndices.size());

    const quint32 vbSize = static_cast<quint32>(cameraVerts.size()) * sizeof(ColoredVertex);
    const quint32 ibSize = static_cast<quint32>(cameraIndices.size()) * sizeof(quint32);

    _vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbSize));
    _vertexBuffer->create();

    _indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibSize));
    _indexBuffer->create();

    batch->uploadStaticBuffer(_vertexBuffer.get(), cameraVerts.constData());
    batch->uploadStaticBuffer(_indexBuffer.get(), cameraIndices.constData());
}

void SfmCameraRenderable::buildSelectedGeometry(QRhiResourceUpdateBatch* batch)
{
    _selectedVertexBuffer.reset();
    _selectedIndexBuffer.reset();
    _selectedIndexCount = 0;
    _selectedGeometryDirty = false;

    if (!_selectedCameraIndex || *_selectedCameraIndex >= _cameras.size())
    {
        return;
    }

    const SfmDataCameraInstance& camera = _cameras[*_selectedCameraIndex];
    const float scaledSphereRadius = std::max(sphereRadius * _cameraScale, 0.001f);
    const float scaledDepth = std::max(cameraDepth * _cameraScale, 0.0001f);
    const float fovDegrees = camera.intrinsics->getHorizontalFov() * 180.0 / M_PI;
    const float aspectRatio = static_cast<float>(camera.intrinsics->w()) / static_cast<float>(camera.intrinsics->h());

    QVector<ColoredVertex> selectedVerts;
    QVector<quint32> selectedIndices;

    if (fovDegrees > 180.0f)
    {
        buildSphereWireframeMesh(selectedVerts, selectedIndices, scaledSphereRadius * 1.15f, 1.0f, 1.0f, 1.0f, 8, 12);
    }
    else
    {
        buildCameraWireframeMesh(selectedVerts, selectedIndices, fovDegrees, scaledDepth * 1.08f, aspectRatio);
    }

    QVector<ColoredVertex> transformedVerts;
    QVector<quint32> transformedIndices;
    appendMesh(transformedVerts, transformedIndices, selectedVerts, selectedIndices, toQMatrix4x4(camera.camera_T_world.inverse()));

    if (transformedVerts.empty() || transformedIndices.empty())
    {
        return;
    }

    _selectedIndexCount = static_cast<quint32>(transformedIndices.size());

    const quint32 vbSize = static_cast<quint32>(transformedVerts.size()) * sizeof(ColoredVertex);
    const quint32 ibSize = static_cast<quint32>(transformedIndices.size()) * sizeof(quint32);

    _selectedVertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbSize));
    _selectedVertexBuffer->create();

    _selectedIndexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibSize));
    _selectedIndexBuffer->create();

    batch->uploadStaticBuffer(_selectedVertexBuffer.get(), transformedVerts.constData());
    batch->uploadStaticBuffer(_selectedIndexBuffer.get(), transformedIndices.constData());
}