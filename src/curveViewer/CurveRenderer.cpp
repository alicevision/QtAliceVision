#include "CurveRenderer.hpp"
#include "CurveViewer.hpp"

#include <QFile>
#include <QMatrix4x4>
#include <rhi/qrhi.h>

#include <algorithm>
#include <bit>
#include <cstring>
#include <tuple>
#include <utility>

namespace curveViewer {

namespace {

// warning: matches the uniform buffer layout of CurveLine.vert/frag
constexpr quint32 uniformSize = 128;
constexpr int itemToClipOffset = 0;
constexpr int colorOffset = 64;
constexpr int originOffset = 80;
constexpr int scaleOffset = 96;
constexpr int viewportSizeOffset = 104;
constexpr int halfWidthOffset = 112;
constexpr int heightOffset = 116;

/// Floats per segment: high and low parts of a.xy, b.xy
constexpr std::size_t segmentFloats = 8;
/// Floats per envelope vertex: high and low parts of xy
constexpr std::size_t fillVertexFloats = 4;

/// Split a double into two floats whose sum keeps ~48 bits of precision.
std::pair<float, float> split(double v)
{
    const auto hi = static_cast<float>(v);
    return {hi, static_cast<float>(v - static_cast<double>(hi))};
}

QShader loadShader(const QString& name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
    {
        return QShader::fromSerialized(f.readAll());
    }
    qWarning() << "[CurveViewer] Failed to load shader" << name;
    return {};
}

}  // namespace

struct CurveRenderer::GpuCurve
{
    quint32 segmentCount = 0;
    /// Segment data (a.xy, b.xy high parts, then low parts) waiting for upload
    std::vector<float> pending;
    std::unique_ptr<QRhiBuffer> segments;

    /// Envelope triangle strip, (x, lower) and (x, upper) per sample
    struct Fill
    {
        quint32 vertexCount = 0;
        std::vector<float> pending;
        std::unique_ptr<QRhiBuffer> vertices;
    };
    std::vector<Fill> fills;
};

struct CurveRenderer::Draw
{
    GpuCurve* curve;
    /// Index of the envelope to fill, -1 to draw the curve line
    int fill;
    QColor color;
    float halfWidth;
    float origin[4];  ///< View origin (xMin, yMin) high parts, then low parts
    float scale[2];
    float height;
};

CurveLayer::CurveLayer(CurveViewer* viewer)
  : QQuickRhiItem(viewer),
    _viewer(viewer)
{
    // The texture is composed over the grid
    setAlphaBlending(true);
}

QQuickRhiItemRenderer* CurveLayer::createRenderer() { return new CurveRenderer; }

CurveRenderer::CurveRenderer() = default;

// QRhi defers the release of native resources still in use by in-flight frames
CurveRenderer::~CurveRenderer() = default;

void CurveRenderer::addCurve(const Curve& curve)
{
    if (_curves.count(curve.id))
    {
        return;
    }

    auto gpu = std::make_unique<GpuCurve>();
    const std::size_t n = curve.x.size();
    if (n > 0)
    {
        // A single sample is drawn as a degenerated segment
        const std::size_t segmentCount = n == 1 ? 1 : n - 1;
        gpu->segmentCount = static_cast<quint32>(segmentCount);
        gpu->pending.resize(segmentCount * segmentFloats);
        float* data = gpu->pending.data();
        for (std::size_t s = 0; s < segmentCount; ++s)
        {
            const std::size_t i1 = std::min(s + 1, n - 1);
            const double coords[4] = {curve.x[s], curve.y[s], curve.x[i1], curve.y[i1]};
            for (std::size_t k = 0; k < 4; ++k)
            {
                std::tie(data[s * segmentFloats + k], data[s * segmentFloats + 4 + k]) = split(coords[k]);
            }
        }
    }
    for (const Envelope& envelope : curve.envelopes)
    {
        auto& fill = gpu->fills.emplace_back();
        if (n < 2)
        {
            continue;
        }
        fill.vertexCount = static_cast<quint32>(2 * n);
        fill.pending.resize(2 * n * fillVertexFloats);
        float* data = fill.pending.data();
        for (std::size_t i = 0; i < n; ++i)
        {
            for (const double y : {envelope.lower[i], envelope.upper[i]})
            {
                std::tie(data[0], data[2]) = split(curve.x[i]);
                std::tie(data[1], data[3]) = split(y);
                data += fillVertexFloats;
            }
        }
    }
    _curves.emplace(curve.id, std::move(gpu));
}

void CurveRenderer::retainCurves(const std::unordered_set<quint64>& ids)
{
    for (auto it = _curves.begin(); it != _curves.end();)
    {
        if (!ids.count(it->first))
        {
            it = _curves.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void CurveRenderer::clearDraws() { _draws.clear(); }

void CurveRenderer::addDraw(quint64 id, const QColor& color, float lineWidth, double sx, double sy, double xMin, double yMin, double height)
{
    const auto it = _curves.find(id);
    if (it == _curves.end() || it->second->segmentCount == 0)
    {
        return;
    }
    pushDraw(it->second.get(), -1, color, lineWidth, sx, sy, xMin, yMin, height);
}

void CurveRenderer::addFillDraw(quint64 id, int envelope, const QColor& color, double sx, double sy, double xMin, double yMin, double height)
{
    const auto it = _curves.find(id);
    if (it == _curves.end() || envelope < 0 || envelope >= static_cast<int>(it->second->fills.size()) ||
        it->second->fills[static_cast<std::size_t>(envelope)].vertexCount == 0)
    {
        return;
    }
    pushDraw(it->second.get(), envelope, color, 0.0f, sx, sy, xMin, yMin, height);
}

void CurveRenderer::pushDraw(GpuCurve* curve, int fill, const QColor& color, float lineWidth, double sx, double sy, double xMin, double yMin, double height)
{
    // Data to item: ((p - origin) * scale) with Y up, origin subtracted on the GPU in double-float precision
    Draw draw{curve, fill, color, 0.5f * lineWidth, {}, {static_cast<float>(sx), static_cast<float>(sy)}, static_cast<float>(height)};
    std::tie(draw.origin[0], draw.origin[2]) = split(xMin);
    std::tie(draw.origin[1], draw.origin[3]) = split(yMin);
    _draws.push_back(draw);
}

void CurveRenderer::synchronize(QQuickRhiItem* item)
{
    _itemSize = item->size();
    static_cast<CurveLayer*>(item)->viewer()->synchronizeCurves(*this);
}

void CurveRenderer::initialize([[maybe_unused]] QRhiCommandBuffer* cb)
{
    // Called when the render target is created again (resize, ...): the pipelines must match its render pass
    if (renderTarget()->renderPassDescriptor() != _renderPass)
    {
        _pipeline.reset();
        _fillPipeline.reset();
    }
}

void CurveRenderer::render(QRhiCommandBuffer* cb)
{
    QRhi* r = rhi();
    QRhiRenderTarget* rt = renderTarget();
    QRhiResourceUpdateBatch* updates = r->nextResourceUpdateBatch();

    // Instancing is not available with OpenGL contexts older than 3.3 (the Qt Quick default format is 2.0)
    const bool instancing = r->isFeatureSupported(QRhi::Instancing);

    if (instancing && !_quadVertices)
    {
        static const float corners[] = {0.f, 1.f, 2.f, 3.f};
        static const quint16 indices[] = {0, 1, 2, 2, 1, 3};
        _quadVertices.reset(r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(corners)));
        _quadVertices->create();
        _quadIndices.reset(r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)));
        _quadIndices->create();
        updates->uploadStaticBuffer(_quadVertices.get(), corners);
        updates->uploadStaticBuffer(_quadIndices.get(), indices);
    }

    // Upload new curves once
    quint32 maxSegmentCount = 0;
    for (auto& [id, curve] : _curves)
    {
        for (auto& fill : curve->fills)
        {
            if (fill.pending.empty())
            {
                continue;
            }
            const auto size = static_cast<quint32>(fill.pending.size() * sizeof(float));
            fill.vertices.reset(r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, size));
            fill.vertices->create();
            updates->uploadStaticBuffer(fill.vertices.get(), fill.pending.data());
            fill.pending = {};
        }

        maxSegmentCount = std::max(maxSegmentCount, curve->segmentCount);
        if (curve->pending.empty())
        {
            continue;
        }

        if (!instancing)
        {
            // Expand each segment into the 4 vertices (corner, segment) of its quad
            constexpr std::size_t vertexFloats = 1 + segmentFloats;
            std::vector<float> expanded(static_cast<std::size_t>(curve->segmentCount) * 4 * vertexFloats);
            for (std::size_t s = 0; s < curve->segmentCount; ++s)
            {
                for (std::size_t c = 0; c < 4; ++c)
                {
                    float* v = expanded.data() + (s * 4 + c) * vertexFloats;
                    v[0] = static_cast<float>(c);
                    std::memcpy(v + 1, curve->pending.data() + s * segmentFloats, segmentFloats * sizeof(float));
                }
            }
            curve->pending = std::move(expanded);
        }

        const auto size = static_cast<quint32>(curve->pending.size() * sizeof(float));
        curve->segments.reset(r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, size));
        curve->segments->create();
        updates->uploadStaticBuffer(curve->segments.get(), curve->pending.data());
        curve->pending = {};
    }

    // Without instancing, all curves share the same index buffer, sized for the largest curve
    if (!instancing && maxSegmentCount > _segmentIndicesCapacity)
    {
        _segmentIndicesCapacity = std::bit_ceil(maxSegmentCount);
        std::vector<quint32> indices(static_cast<std::size_t>(_segmentIndicesCapacity) * 6);
        for (quint32 seg = 0; seg < _segmentIndicesCapacity; ++seg)
        {
            const quint32 v = seg * 4;
            const quint32 quad[6] = {v, v + 1, v + 2, v + 2, v + 1, v + 3};
            std::memcpy(indices.data() + seg * 6, quad, sizeof(quad));
        }
        _segmentIndices.reset(r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, static_cast<quint32>(indices.size() * sizeof(quint32))));
        _segmentIndices->create();
        updates->uploadStaticBuffer(_segmentIndices.get(), indices.data());
    }

    // One uniform buffer per draw
    while (_drawResources.size() < _draws.size())
    {
        std::unique_ptr<QRhiBuffer> ubuf(r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, uniformSize));
        ubuf->create();
        std::unique_ptr<QRhiShaderResourceBindings> srb(r->newShaderResourceBindings());
        srb->setBindings({QRhiShaderResourceBinding::uniformBuffer(
          0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf.get())});
        srb->create();
        _drawResources.emplace_back(std::move(ubuf), std::move(srb));
    }

    const QSize targetSize = rt->pixelSize();
    const float viewportSize[2] = {static_cast<float>(targetSize.width()), static_cast<float>(targetSize.height())};
    // Item coordinates (Y down) to the clip space of the backend
    QMatrix4x4 itemToClip = r->clipSpaceCorrMatrix();
    itemToClip.ortho(0.f, static_cast<float>(_itemSize.width()), static_cast<float>(_itemSize.height()), 0.f, -1.f, 1.f);

    for (std::size_t i = 0; i < _draws.size(); ++i)
    {
        const Draw& draw = _draws[i];
        char data[uniformSize] = {};
        std::memcpy(data + itemToClipOffset, itemToClip.constData(), 64);
        // The item opacity is applied when the texture is composed
        const float a = draw.color.alphaF();
        const float color[4] = {draw.color.redF() * a, draw.color.greenF() * a, draw.color.blueF() * a, a};
        std::memcpy(data + colorOffset, color, 16);
        std::memcpy(data + viewportSizeOffset, viewportSize, 8);
        std::memcpy(data + originOffset, draw.origin, 16);
        std::memcpy(data + scaleOffset, draw.scale, 8);
        std::memcpy(data + halfWidthOffset, &draw.halfWidth, 4);
        std::memcpy(data + heightOffset, &draw.height, 4);
        updates->updateDynamicBuffer(_drawResources[i].first.get(), 0, uniformSize, data);
    }

    if (instancing != _instancing)
    {
        _pipeline.reset();
        _fillPipeline.reset();
        _instancing = instancing;
    }
    if (!_pipeline && !_drawResources.empty())
    {
        createPipelines();
    }

    cb->beginPass(rt, Qt::transparent, {1.0f, 0}, updates);
    if (!_pipeline)
    {
        cb->endPass();
        return;
    }

    // Viewport is set again after each pipeline change
    QRhiGraphicsPipeline* current = nullptr;
    const auto bindPipeline = [&](QRhiGraphicsPipeline* pipeline) {
        if (pipeline == current)
        {
            return;
        }
        current = pipeline;
        cb->setGraphicsPipeline(pipeline);
        cb->setViewport(QRhiViewport(0, 0, viewportSize[0], viewportSize[1]));
    };

    for (std::size_t i = 0; i < _draws.size(); ++i)
    {
        const Draw& draw = _draws[i];
        if (draw.fill >= 0)
        {
            const auto& fill = draw.curve->fills[static_cast<std::size_t>(draw.fill)];
            if (!fill.vertices)
            {
                continue;
            }
            bindPipeline(_fillPipeline.get());
            cb->setShaderResources(_drawResources[i].second.get());
            const QRhiCommandBuffer::VertexInput input(fill.vertices.get(), 0);
            cb->setVertexInput(0, 1, &input);
            cb->draw(fill.vertexCount);
            continue;
        }

        if (!draw.curve->segments)
        {
            continue;
        }
        bindPipeline(_pipeline.get());
        cb->setShaderResources(_drawResources[i].second.get());
        if (_instancing)
        {
            const QRhiCommandBuffer::VertexInput inputs[] = {{_quadVertices.get(), 0}, {draw.curve->segments.get(), 0}};
            cb->setVertexInput(0, 2, inputs, _quadIndices.get(), 0, QRhiCommandBuffer::IndexUInt16);
            cb->drawIndexed(6, draw.curve->segmentCount);
        }
        else
        {
            const QRhiCommandBuffer::VertexInput input(draw.curve->segments.get(), 0);
            cb->setVertexInput(0, 1, &input, _segmentIndices.get(), 0, QRhiCommandBuffer::IndexUInt32);
            cb->drawIndexed(6 * draw.curve->segmentCount);
        }
    }
    cb->endPass();
}

void CurveRenderer::createPipelines()
{
    QRhiRenderTarget* rt = renderTarget();
    _renderPass = rt->renderPassDescriptor();

    const auto createPipeline = [&](const QString& shader, const QRhiVertexInputLayout& layout, QRhiGraphicsPipeline::Topology topology) {
        std::unique_ptr<QRhiGraphicsPipeline> pipeline(rhi()->newGraphicsPipeline());
        pipeline->setShaderStages({
          {QRhiShaderStage::Vertex, loadShader(QStringLiteral(":/curveViewerShaders/%1.vert.qsb").arg(shader))},
          {QRhiShaderStage::Fragment, loadShader(QStringLiteral(":/curveViewerShaders/%1.frag.qsb").arg(shader))},
        });
        pipeline->setVertexInputLayout(layout);
        pipeline->setTopology(topology);

        // Premultiplied alpha blending, as expected by the item when composing the texture
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::One;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        pipeline->setTargetBlends({blend});

        pipeline->setSampleCount(rt->sampleCount());
        pipeline->setShaderResourceBindings(_drawResources.front().second.get());
        pipeline->setRenderPassDescriptor(_renderPass);
        pipeline->create();
        return pipeline;
    };

    QRhiVertexInputLayout lineLayout;
    if (_instancing)
    {
        lineLayout.setBindings({
          {sizeof(float)},                                                       // quad corner
          {segmentFloats * sizeof(float), QRhiVertexInputBinding::PerInstance},  // segment
        });
        lineLayout.setAttributes({
          {0, 0, QRhiVertexInputAttribute::Float, 0},
          {1, 1, QRhiVertexInputAttribute::Float4, 0},
          {1, 2, QRhiVertexInputAttribute::Float4, 4 * sizeof(float)},
        });
    }
    else
    {
        lineLayout.setBindings({{(1 + segmentFloats) * sizeof(float)}});
        lineLayout.setAttributes({
          {0, 0, QRhiVertexInputAttribute::Float, 0},
          {0, 1, QRhiVertexInputAttribute::Float4, sizeof(float)},
          {0, 2, QRhiVertexInputAttribute::Float4, 5 * sizeof(float)},
        });
    }
    _pipeline = createPipeline(QStringLiteral("CurveLine"), lineLayout, QRhiGraphicsPipeline::Triangles);

    QRhiVertexInputLayout fillLayout;
    fillLayout.setBindings({{fillVertexFloats * sizeof(float)}});
    fillLayout.setAttributes({
      {0, 0, QRhiVertexInputAttribute::Float2, 0},                  // high parts
      {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)},  // low parts
    });
    _fillPipeline = createPipeline(QStringLiteral("CurveFill"), fillLayout, QRhiGraphicsPipeline::TriangleStrip);
}

}  // namespace curveViewer
