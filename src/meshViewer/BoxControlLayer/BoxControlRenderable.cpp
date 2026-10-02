#include <BoxControlLayer/BoxControlRenderable.hpp>
#include <BoxControlLayer/BoxControlGeometry.hpp>
#include <BoxControlLayer/BoxControlLayer.hpp>
#include <Core/SceneView.hpp>

#include <QFile>

namespace {

QShader loadBoxControlShader(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        qFatal("Cannot open shader %s", qPrintable(path));
    }

    return QShader::fromSerialized(f.readAll());
}

/** Size of the uniform buffer: mvp (mat4) + params (vec4). */
constexpr quint32 uniformSize = 64 + 16;

/** Opacity of the box faces. */
constexpr float faceAlpha = 0.15f;

}  // namespace

void BoxControlRenderable::sync(LayerItem* layer, SceneView* view)
{
    Q_UNUSED(view)

    const BoxControlLayer* boxLayer = static_cast<BoxControlLayer*>(layer);
    _translation = boxLayer->translation();
    _orientation = boxLayer->orientation();
    _scale = boxLayer->scale();
    _gizmoSize = boxLayer->gizmoSize();
    _gizmoVisible = boxLayer->gizmoVisible();
    _highlightedHandle = boxLayer->highlightedHandle();
}

void BoxControlRenderable::initialize(QRhi* rhi, QRhiRenderPassDescriptor* rpDesc)
{
    if (_rhi != rhi)
    {
        _rhi = rhi;
        _rpDesc = rpDesc;
        _pipelineDirty = true;
        _boxGeomDirty = true;
        _builtGizmoScale = -1.f;
        _gizmoVertexCapacity = 0;
        _faces = {};
        _edges = {};
        _gizmo = {};
    }

    if (_pipelineDirty)
    {
        buildPipelines();
    }
}

void BoxControlRenderable::buildPipelines()
{
    const QShader vert = loadBoxControlShader(":/shaders/colorAlpha.vert.qsb");
    const QShader frag = loadBoxControlShader(":/shaders/colorAlpha.frag.qsb");

    // Vertex layout: position (vec3) + color (vec3)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({QRhiVertexInputBinding(sizeof(ColoredVertex))});
    inputLayout.setAttributes({QRhiVertexInputAttribute(0, 0, QRhiVertexInputAttribute::Float3, 0),
                               QRhiVertexInputAttribute(0, 1, QRhiVertexInputAttribute::Float3, sizeof(float) * 3)});

    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    auto setup = [&](DrawItem& item, QRhiGraphicsPipeline::Topology topology, bool depthTest, bool depthWrite) {
        item.uniformBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, uniformSize));
        item.uniformBuffer->create();

        item.srb.reset(_rhi->newShaderResourceBindings());
        item.srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, item.uniformBuffer.get())});
        item.srb->create();

        item.pipeline.reset(_rhi->newGraphicsPipeline());
        item.pipeline->setShaderStages({
          {QRhiShaderStage::Vertex, vert},
          {QRhiShaderStage::Fragment, frag},
        });
        item.pipeline->setVertexInputLayout(inputLayout);
        item.pipeline->setShaderResourceBindings(item.srb.get());
        item.pipeline->setRenderPassDescriptor(_rpDesc);
        item.pipeline->setTopology(topology);
        item.pipeline->setDepthTest(depthTest);
        item.pipeline->setDepthWrite(depthWrite);
        item.pipeline->setCullMode(QRhiGraphicsPipeline::None);
        item.pipeline->setTargetBlends({blend});
        item.pipeline->create();
    };

    // Box faces: translucent, tested against the scene but not occluding it
    setup(_faces, QRhiGraphicsPipeline::Triangles, true, false);
    // Box edges: tested against the scene
    setup(_edges, QRhiGraphicsPipeline::Lines, true, false);
    // Gizmo: depth tested against itself only, its depth range being squeezed in front of the scene (see render())
    setup(_gizmo, QRhiGraphicsPipeline::Triangles, true, true);

    _pipelineDirty = false;
}

void BoxControlRenderable::buildBoxGeometry(QRhiResourceUpdateBatch* batch)
{
    QVector<ColoredVertex> verts;
    QVector<quint32> indices;

    boxControl::buildBoxFaces(verts, indices);
    _faces.count = static_cast<quint32>(indices.size());
    _faces.vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, verts.size() * sizeof(ColoredVertex)));
    _faces.vertexBuffer->create();
    _faces.indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, indices.size() * sizeof(quint32)));
    _faces.indexBuffer->create();
    batch->uploadStaticBuffer(_faces.vertexBuffer.get(), verts.constData());
    batch->uploadStaticBuffer(_faces.indexBuffer.get(), indices.constData());

    boxControl::buildBoxEdges(verts);
    _edges.count = static_cast<quint32>(verts.size());
    _edges.vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, verts.size() * sizeof(ColoredVertex)));
    _edges.vertexBuffer->create();
    batch->uploadStaticBuffer(_edges.vertexBuffer.get(), verts.constData());

    _boxGeomDirty = false;
}

void BoxControlRenderable::buildGizmoMesh(QVector<ColoredVertex>& verts, QVector<quint32>& indices, float gizmoScale) const
{
    using namespace boxControl;

    const QVector3D white(1.f, 1.f, 1.f);
    auto colorOf = [&](int handle, int axis) { return handle == _highlightedHandle ? white : axisColor(axis); };

    verts.clear();
    indices.clear();

    appendSphere(verts, indices, QVector3D(), centerRadius, white);

    for (int axis = 0; axis < 3; ++axis)
    {
        const QVector3D dir = axisVector(axis);

        // Scale handle: shaft + cube
        const QVector3D scaleColor = colorOf(BoxControlLayer::ScaleX + axis, axis);
        appendTube(verts, indices, dir, shaftStart, shaftEnd, shaftRadius, shaftRadius, scaleColor);
        appendCube(verts, indices, dir * shaftEnd, scaleCubeEdge, scaleColor);

        // Translate handle: cone placed outside the box, hence in world units divided by the gizmo scale
        const float coneStart = translateHandleDistance(_scale[axis]) / gizmoScale;
        appendTube(verts, indices, dir, coneStart, coneStart + coneLength, coneRadius, 0.001f, colorOf(BoxControlLayer::TranslateX + axis, axis));

        // Rotate handle: ring around the axis
        appendTorus(verts, indices, dir, torusRadiusForAxis(axis), torusMinorRadius, colorOf(BoxControlLayer::RotateX + axis, axis));

        // Face handles: placed in world units from the box faces, hence divided by the gizmo scale
        for (int s = 0; s < 2; ++s)
        {
            const float sign = s == 0 ? 1.f : -1.f;
            const QVector3D center = dir * (sign * (_scale[axis] + faceOffset) / gizmoScale);
            appendSphere(verts, indices, center, faceRadius, colorOf(BoxControlLayer::FaceXPos + 2 * axis + s, axis));
        }
    }
}

void BoxControlRenderable::updateGizmoGeometry(QRhiResourceUpdateBatch* batch, float gizmoScale)
{
    const bool scaleChanged = std::abs(gizmoScale - _builtGizmoScale) > 1e-4f * std::abs(gizmoScale);
    if (!scaleChanged && _builtScale == _scale && _builtHighlight == _highlightedHandle && _gizmo.vertexBuffer)
    {
        return;
    }

    QVector<ColoredVertex> verts;
    QVector<quint32> indices;
    buildGizmoMesh(verts, indices, gizmoScale);

    // The topology never changes: buffers are created once, then only vertices are updated.
    const quint32 vbSize = static_cast<quint32>(verts.size() * sizeof(ColoredVertex));
    if (!_gizmo.vertexBuffer || _gizmoVertexCapacity != vbSize)
    {
        _gizmo.vertexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, vbSize));
        _gizmo.vertexBuffer->create();
        _gizmo.indexBuffer.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, indices.size() * sizeof(quint32)));
        _gizmo.indexBuffer->create();
        batch->uploadStaticBuffer(_gizmo.indexBuffer.get(), indices.constData());
        _gizmo.count = static_cast<quint32>(indices.size());
        _gizmoVertexCapacity = vbSize;
    }

    batch->updateDynamicBuffer(_gizmo.vertexBuffer.get(), 0, vbSize, verts.constData());

    _builtGizmoScale = gizmoScale;
    _builtScale = _scale;
    _builtHighlight = _highlightedHandle;
}

void BoxControlRenderable::updateUniforms(QRhiResourceUpdateBatch* batch, DrawItem& item, const QMatrix4x4& mvp, float alpha)
{
    const float params[4] = {alpha, 0.f, 0.f, 0.f};
    batch->updateDynamicBuffer(item.uniformBuffer.get(), 0, 64, mvp.constData());
    batch->updateDynamicBuffer(item.uniformBuffer.get(), 64, sizeof(params), params);
}

void BoxControlRenderable::prepare(QRhiResourceUpdateBatch* batch, const SceneState& state)
{
    if (!_rhi || _pipelineDirty)
    {
        return;
    }

    if (_boxGeomDirty)
    {
        buildBoxGeometry(batch);
    }

    QMatrix4x4 boxModel;
    boxModel.translate(_translation);
    boxModel.rotate(_orientation);
    boxModel.scale(_scale);
    const QMatrix4x4 boxMvp = state.viewProjection * boxModel;
    updateUniforms(batch, _faces, boxMvp, faceAlpha);
    updateUniforms(batch, _edges, boxMvp, 1.f);

    if (_gizmoVisible)
    {
        const float gizmoScale = BoxControlLayer::gizmoScaleFor(state.viewProjection, state.projectionScaleY, _translation, _gizmoSize);
        if (gizmoScale > 0.f)
        {
            updateGizmoGeometry(batch, gizmoScale);

            QMatrix4x4 gizmoModel;
            gizmoModel.translate(_translation);
            gizmoModel.rotate(_orientation);
            gizmoModel.scale(gizmoScale);
            updateUniforms(batch, _gizmo, state.viewProjection * gizmoModel, 1.f);
        }
    }
}

void BoxControlRenderable::draw(QRhiCommandBuffer* cb, const SceneState& state, DrawItem& item, bool indexed, float maxDepth)
{
    if (!item.pipeline || !item.vertexBuffer || item.count == 0 || (indexed && !item.indexBuffer))
    {
        return;
    }

    cb->setGraphicsPipeline(item.pipeline.get());
    cb->setViewport({0, 0, float(state.viewportWidth), float(state.viewportHeight), 0.f, maxDepth});
    cb->setShaderResources(item.srb.get());

    const QRhiCommandBuffer::VertexInput vb(item.vertexBuffer.get(), 0);
    if (indexed)
    {
        cb->setVertexInput(0, 1, &vb, item.indexBuffer.get(), 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(item.count);
    }
    else
    {
        cb->setVertexInput(0, 1, &vb);
        cb->draw(item.count);
    }
}

void BoxControlRenderable::render(QRhiCommandBuffer* cb, const SceneState& state)
{
    draw(cb, state, _faces, true);
    draw(cb, state, _edges, false);

    if (_gizmoVisible)
    {
        // Map the gizmo depth to [0, 0.001] so that it is drawn on top of the scene while still hiding its own back parts
        draw(cb, state, _gizmo, true, 0.001f);
    }
}
