#pragma once

#include "CurveModel.hpp"

#include <QColor>
#include <QQuickRhiItem>

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class QRhiBuffer;
class QRhiGraphicsPipeline;
class QRhiRenderPassDescriptor;
class QRhiShaderResourceBindings;

namespace curveViewer {

class CurveViewer;

/**
 * @brief Item drawing the curves of a CurveViewer into its own texture (see CurveRenderer).
 *
 * The texture is only rendered again when this item is updated: changes of the other layers of the viewer
 * (playhead, bands, ...) or of any other item of the window just compose the previous result.
 */
class CurveLayer : public QQuickRhiItem
{
  public:
    explicit CurveLayer(CurveViewer* viewer);

    CurveViewer* viewer() const { return _viewer; }

  protected:
    QQuickRhiItemRenderer* createRenderer() override;

  private:
    CurveViewer* _viewer;
};

/**
 * @brief Renderer drawing antialiased polylines with a constant width in pixels, using QRhi directly.
 *
 * Curves are not regular scene graph geometry nodes on purpose: any structural change in the window
 * (a node added anywhere, a new glyph, ...) makes the scene graph renderer rebuild its batches and
 * upload their vertex data again, which stalls for large curves. Here, each curve sample buffer is
 * uploaded once to the GPU and kept until the curve is removed.
 *
 * Each segment is a quad extruded in screen space by the vertex shader (see CurveLine.vert), drawn with
 * instancing when available (32 bytes per segment), otherwise expanded on the CPU (144 bytes per segment).
 * Coordinates are stored as high/low float pairs and the view origin is subtracted on the GPU before
 * any single precision operation, so that deep zooms stay accurate whatever the data magnitude.
 * Per frame, only one small uniform buffer per drawn curve is updated.
 *
 * Curve envelopes are triangle strips between their lower and upper bounds (see CurveFill.vert),
 * stored the same way and drawn with a second pipeline.
 */
class CurveRenderer : public QQuickRhiItemRenderer
{
  public:
    CurveRenderer();
    ~CurveRenderer() override;

    /// @name Synchronization, called from CurveViewer::synchronizeCurves
    /// @{

    /// Copy the samples of a curve if it is not known yet.
    void addCurve(const Curve& curve);
    /// Release the curves whose id is not in ids.
    void retainCurves(const std::unordered_set<quint64>& ids);

    void clearDraws();
    /**
     * @brief Draw a curve on top of the previous draws.
     * @param id Curve id, must have been added.
     * @param color Line color.
     * @param lineWidth Line width in device pixels.
     * @param sx, sy, xMin, yMin, height Data to item coordinates mapping (Y up).
     */
    void addDraw(quint64 id, const QColor& color, float lineWidth, double sx, double sy, double xMin, double yMin, double height);
    /**
     * @brief Fill an envelope of a curve on top of the previous draws.
     * @param envelope Index of the envelope in the curve.
     * @see addDraw for the other parameters.
     */
    void addFillDraw(quint64 id, int envelope, const QColor& color, double sx, double sy, double xMin, double yMin, double height);
    /// @}

  protected:
    void initialize(QRhiCommandBuffer* cb) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* cb) override;

  private:
    struct GpuCurve;
    struct Draw;

    void pushDraw(GpuCurve* curve, int fill, const QColor& color, float lineWidth, double sx, double sy, double xMin, double yMin, double height);
    void createPipelines();

    /// Item size, in item coordinates
    QSizeF _itemSize;

    std::unordered_map<quint64, std::unique_ptr<GpuCurve>> _curves;
    std::vector<Draw> _draws;
    /// Uniform buffers and bindings, one per draw, reused between frames
    std::vector<std::pair<std::unique_ptr<QRhiBuffer>, std::unique_ptr<QRhiShaderResourceBindings>>> _drawResources;

    std::unique_ptr<QRhiBuffer> _quadVertices;
    std::unique_ptr<QRhiBuffer> _quadIndices;
    /// Without instancing: indices of the expanded quads, shared by all curves
    std::unique_ptr<QRhiBuffer> _segmentIndices;
    quint32 _segmentIndicesCapacity = 0;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> _fillPipeline;
    /// Render pass the pipelines were created for
    QRhiRenderPassDescriptor* _renderPass = nullptr;
    bool _instancing = true;
};

}  // namespace curveViewer
