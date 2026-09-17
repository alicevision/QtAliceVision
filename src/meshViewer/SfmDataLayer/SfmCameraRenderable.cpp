#include <SfmDataLayer/SfmCameraRenderable.hpp>

#include <Geometry/Geometry.hpp>

#include <QFile>

#include <algorithm>
#include <cmath>
#include <utility>

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

/// Appends the 16 column-major floats of @p m to @p dst (matches GLSL mat4-from-columns layout).
void appendMat4(QVector<float>& dst, const QMatrix4x4& m)
{
    const float* data = m.constData();
    for (int i = 0; i < 16; ++i)
    {
        dst.append(data[i]);
    }
}

}  // namespace

void SfmCameraRenderable::setData(std::vector<SfmDataCameraInstance> cameras)
{
    _cameras.clear();

    for (const auto& item : cameras)
    {
        _cameras[item.viewId] = item;
    }

    _instancesDirty = true;
}

void SfmCameraRenderable::setCameraScale(float cameraScale)
{
    if (_cameraScale == cameraScale)
    {
        return;
    }

    // Applied as a shader uniform (see prepare()), so changing it never requires rebuilding
    // per-instance transforms or re-uploading geometry.
    _cameraScale = cameraScale;
    _scaleDirty = true;
}

void SfmCameraRenderable::setSelectedCamera(std::optional<aliceVision::IndexT> cameraIndex)
{
    if (_selectedCameraIndex == cameraIndex)
    {
        return;
    }

    _selectedCameraIndex = cameraIndex;
    _scaleDirty = true;
}

void SfmCameraRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _staticGeometryDirty = true;
        _instancesDirty = true;
        _scaleDirty = true;

        _pyramid = SfmCameraShapeGpu();
        _sphere = SfmCameraShapeGpu();
    }

    if (_pipelineDirty)
    {
        buildPipeline();
    }
}

void SfmCameraRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (_staticGeometryDirty)
    {
        buildStaticGeometry(_pyramid, batch, false);
        buildStaticGeometry(_sphere, batch, true);
        _staticGeometryDirty = false;
    }

    if (_instancesDirty)
    {
        buildInstanceData(batch);
        _instancesDirty = false;
        _scaleDirty = false;
    }
    else if (_scaleDirty)
    {
        updateSelectionBuffers(batch);
        _scaleDirty = false;
    }

    // Minimum runtime scale that keeps each shape's base unit size (cameraDepth / sphereRadius)
    // from collapsing to a degenerate zero-size shape, matching the previous CPU-side clamps.
    const float pyramidScale = std::max(_cameraScale, 0.0001f / cameraDepth);
    const float sphereScale = std::max(_cameraScale, 0.001f / sphereRadius);

    for (const auto& entry : {std::pair{&_pyramid, pyramidScale}, std::pair{&_sphere, sphereScale}})
    {
        SfmCameraShapeGpu* shape = entry.first;
        if (!shape->uniformBuffer || shape->instanceCount == 0)
        {
            continue;
        }

        // std140 layout: mat4 viewProjection (64 bytes) followed by highlightScale and
        // cameraScale floats, rounded up to a 16-byte multiple.
        float payload[16] = {};
        std::copy_n(state.viewProjection.constData(), 16, payload);

        batch->updateDynamicBuffer(shape->uniformBuffer.get(), 0, sizeof(payload), payload);
    }
}

void SfmCameraRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    if (_cameraScale <= 0.0f)
    {
        return;
    }
    renderShape(cb, state, _pyramid);
    renderShape(cb, state, _sphere);
}

void SfmCameraRenderable::renderShape(QRhiCommandBuffer* cb, const SceneState& state, SfmCameraShapeGpu& shape)
{
    if (!shape.pipeline || !shape.vertexBuffer || !shape.indexBuffer || !shape.transformBuffer || !shape.scalesBuffer || shape.instanceCount == 0 ||
        shape.indexCount == 0)
    {
        return;
    }

    cb->setGraphicsPipeline(shape.pipeline.get());
    cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight)});
    cb->setShaderResources(shape.srb.get());

    const QRhiCommandBuffer::VertexInput vbs[] = {
      {shape.vertexBuffer.get(), 0},
      {shape.transformBuffer.get(), 0},
      {shape.scalesBuffer.get(), 0},
    };
    cb->setVertexInput(0, 3, vbs, shape.indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
    cb->drawIndexed(shape.indexCount, shape.instanceCount);
}

void SfmCameraRenderable::buildPipeline()
{
    buildShapePipeline(_pyramid);
    buildShapePipeline(_sphere);

    _pipelineDirty = false;
}

void SfmCameraRenderable::buildShapePipeline(SfmCameraShapeGpu& shape)
{
    shape.pipeline.reset();
    shape.srb.reset();
    shape.uniformBuffer.reset();

    // std140 UBO: mat4 viewProjection (64 bytes) , padded to 16 bytes.
    shape.uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    shape.uniformBuffer->create();

    shape.srb.reset(_rhi->newShaderResourceBindings());
    shape.srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, shape.uniformBuffer.get())});
    shape.srb->create();

    QShader vert = loadSfmCameraShader(":/shaders/instancedCameraShape.vert.qsb");
    QShader frag = loadSfmCameraShader(":/shaders/simpleColor.frag.qsb");

    // Binding 0: per-vertex data (local position + color).
    // Binding 1: per-instance camera transform (mat4, as 4 consecutive vec4 attributes).
    // Binding 2: per-instance selection flag (float).
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(ColoredVertex)),
                             QRhiVertexInputBinding(sizeof(float) * 16, QRhiVertexInputBinding::PerInstance),
                             QRhiVertexInputBinding(sizeof(float), QRhiVertexInputBinding::PerInstance)});
    inputLayout.setAttributes({
      QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
      QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, sizeof(float) * 3),
      QRhiVertexInputAttribute(1, 2, QRhiVertexInputAttribute::Float4, sizeof(float) * 0),
      QRhiVertexInputAttribute(1, 3, QRhiVertexInputAttribute::Float4, sizeof(float) * 4),
      QRhiVertexInputAttribute(1, 4, QRhiVertexInputAttribute::Float4, sizeof(float) * 8),
      QRhiVertexInputAttribute(1, 5, QRhiVertexInputAttribute::Float4, sizeof(float) * 12),
      QRhiVertexInputAttribute(2, 6, QRhiVertexInputAttribute::Float, 0),
    });

    shape.pipeline.reset(_rhi->newGraphicsPipeline());
    shape.pipeline->setShaderStages({
      {QRhiShaderStage::Vertex, vert},
      {QRhiShaderStage::Fragment, frag},
    });
    shape.pipeline->setVertexInputLayout(inputLayout);
    shape.pipeline->setShaderResourceBindings(shape.srb.get());
    shape.pipeline->setRenderPassDescriptor(_rpDesc);
    shape.pipeline->setTopology(QRhiGraphicsPipeline::Lines);
    shape.pipeline->setDepthTest(true);
    shape.pipeline->setDepthWrite(true);
    shape.pipeline->setCullMode(QRhiGraphicsPipeline::None);
    shape.pipeline->create();
}

void SfmCameraRenderable::buildStaticGeometry(SfmCameraShapeGpu& shape, QRhiResourceUpdateBatch* batch, bool sphereShape)
{
    shape.vertexBuffer.reset();
    shape.indexBuffer.reset();
    shape.indexCount = 0;

    QVector<ColoredVertex> verts;
    QVector<quint32> indices;

    if (sphereShape)
    {
        // Canonical unit sphere (radius = 1); real cameras apply a uniform per-instance scale.
        buildSphereWireframeMesh(verts, indices, 1.0f, 1.0f, 1.0f, 1.0f, 8, 12);
    }
    else
    {
        // Canonical unit pyramid (fov = 90 deg, depth = 1, aspect = 1 => half-width = half-height
        // = depth = 1); real cameras apply a non-uniform per-instance scale derived from their
        // actual fov/aspect/depth to reproduce the exact frustum shape.
        buildCameraWireframeMesh(verts, indices, 90.0f, 1.0f, 1.0f);
    }

    if (verts.empty() || indices.empty())
    {
        return;
    }

    shape.indexCount = static_cast<quint32>(indices.size());

    const quint32 vbSize = static_cast<quint32>(verts.size()) * sizeof(ColoredVertex);
    const quint32 ibSize = static_cast<quint32>(indices.size()) * sizeof(quint32);

    shape.vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vbSize));
    shape.vertexBuffer->create();

    shape.indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, ibSize));
    shape.indexBuffer->create();

    batch->uploadStaticBuffer(shape.vertexBuffer.get(), verts.constData());
    batch->uploadStaticBuffer(shape.indexBuffer.get(), indices.constData());
}

void SfmCameraRenderable::buildInstanceData(QRhiResourceUpdateBatch* batch)
{
    _pyramid.transforms.clear();
    _pyramid.instanceSlots.clear();
    _sphere.transforms.clear();
    _sphere.instanceSlots.clear();

    for (const auto& [viewId, camera] : _cameras)
    {
        const float fovDegrees = camera.intrinsics->getHorizontalFov() * 180.0 / M_PI;

        QMatrix4x4 transform = toQMatrix4x4(camera.camera_T_world.inverse());

        // Note: cameraDepth/sphereRadius are baked here as the *base* (cameraScale == 1) size;
        // the live, runtime-adjustable cameraScale multiplier is applied in the vertex shader
        // (see prepare()), so changing it never requires rebuilding these per-instance transforms.
        if (fovDegrees > 180.0f)
        {
            transform.scale(sphereRadius);

            const quint32 slot = static_cast<quint32>(_sphere.instanceSlots.size());
            _sphere.instanceSlots[viewId] = slot;
            appendMat4(_sphere.transforms, transform);
        }
        else
        {
            const float aspectRatio = static_cast<float>(camera.intrinsics->w()) / static_cast<float>(camera.intrinsics->h());
            const float halfFovRad = std::clamp(fovDegrees, 1.0f, 179.0f) * float(M_PI / 360.0);
            const float halfHeight = cameraDepth * std::tan(halfFovRad);
            const float halfWidth = halfHeight * aspectRatio;

            transform.scale(halfWidth, halfHeight, cameraDepth);

            const quint32 slot = static_cast<quint32>(_pyramid.instanceSlots.size());
            _pyramid.instanceSlots[viewId] = slot;
            appendMat4(_pyramid.transforms, transform);
        }
    }

    _pyramid.instanceCount = static_cast<quint32>(_pyramid.instanceSlots.size());
    _sphere.instanceCount = static_cast<quint32>(_sphere.instanceSlots.size());

    applySelectionToFlags(_pyramid, _selectedCameraIndex);
    applySelectionToFlags(_sphere, _selectedCameraIndex);

    uploadInstanceBuffers(_pyramid, batch);
    uploadInstanceBuffers(_sphere, batch);
}

void SfmCameraRenderable::uploadInstanceBuffers(SfmCameraShapeGpu& shape, QRhiResourceUpdateBatch* batch)
{
    shape.transformBuffer.reset();
    shape.scalesBuffer.reset();

    if (shape.instanceCount == 0)
    {
        return;
    }

    const quint32 transformBufSize = shape.instanceCount * static_cast<quint32>(sizeof(float) * 16);
    shape.transformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, transformBufSize));
    shape.transformBuffer->create();
    batch->uploadStaticBuffer(shape.transformBuffer.get(), shape.transforms.constData());

    const quint32 selectedBufSize = shape.instanceCount * static_cast<quint32>(sizeof(float));
    shape.scalesBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, selectedBufSize));
    shape.scalesBuffer->create();
    batch->updateDynamicBuffer(shape.scalesBuffer.get(), 0, selectedBufSize, shape.scales.constData());
}

void SfmCameraRenderable::updateSelectionBuffers(QRhiResourceUpdateBatch* batch)
{
    applySelectionToFlags(_pyramid, _selectedCameraIndex);
    applySelectionToFlags(_sphere, _selectedCameraIndex);

    for (SfmCameraShapeGpu* shape : {&_pyramid, &_sphere})
    {
        if (!shape->scalesBuffer || shape->scales.isEmpty())
        {
            continue;
        }

        const quint32 size = static_cast<quint32>(shape->scales.size()) * static_cast<quint32>(sizeof(float));
        batch->updateDynamicBuffer(shape->scalesBuffer.get(), 0, size, shape->scales.constData());
    }
}

void SfmCameraRenderable::applySelectionToFlags(SfmCameraShapeGpu& shape, std::optional<aliceVision::IndexT> selected)
{
    if (static_cast<quint32>(shape.scales.size()) != shape.instanceCount)
    {
        shape.scales.assign(shape.instanceCount, _cameraScale);
    }
    else
    {
        std::fill(shape.scales.begin(), shape.scales.end(), _cameraScale);
    }

    if (!selected)
    {
        return;
    }

    const auto it = shape.instanceSlots.find(*selected);
    if (it != shape.instanceSlots.end() && it->second < static_cast<quint32>(shape.scales.size()))
    {
        shape.scales[it->second] = _cameraScale * _selectedScale;
    }
}
