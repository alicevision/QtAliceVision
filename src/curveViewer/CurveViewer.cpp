#include "CurveViewer.hpp"
#include "CurveRenderer.hpp"

#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <unordered_set>
#include <vector>

namespace curveViewer {

namespace {

constexpr double minViewSpan = 1e-9;
constexpr double minRelativeViewSpan = 1e-10;
constexpr double maxViewSpan = 1e15;
constexpr double fitMargin = 0.05;
constexpr double lineWidth = 1.5;
constexpr double currentLineWidth = 2.5;
/// Opacity of the curves which are not current when there is a current curve
constexpr double inactiveOpacity = 0.6;

/**
 * @brief Create an empty scene graph node using one color for all vertices.
 * The drawing mode selects lines or triangles; the node owns its geometry and material.
 * Its color and vertices are populated later by setFlatColor() and setPoints().
 */
QSGGeometryNode* createFlatNode(QSGGeometry::DrawingMode mode)
{
    auto* node = new QSGGeometryNode;
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
    geometry->setDrawingMode(mode);
    geometry->setLineWidth(1.f);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(new QSGFlatColorMaterial);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

/// Change a flat node's material color and notify the scene graph only if it changed.
void setFlatColor(QSGGeometryNode* node, const QColor& color)
{
    auto* material = static_cast<QSGFlatColorMaterial*>(node->material());
    if (material->color() != color)
    {
        material->setColor(color);
        node->markDirty(QSGNode::DirtyMaterial);
    }
}

/// Append two triangles covering the rectangle between (x0, y0) and (x1, y1).
void addRect(std::vector<QPointF>& triangles, double x0, double y0, double x1, double y1)
{ triangles.insert(triangles.end(), {{x0, y0}, {x1, y0}, {x0, y1}, {x0, y1}, {x1, y0}, {x1, y1}}); }

/**
 * @brief Replace a flat node's vertices, reallocating only when their count changes.
 * Coordinates are converted to GPU floats and the geometry is marked dirty.
 * An empty list clears the drawing without deleting the node.
 */
void setPoints(QSGGeometryNode* node, const std::vector<QPointF>& points)
{
    QSGGeometry* geometry = node->geometry();
    if (geometry->vertexCount() != static_cast<int>(points.size()))
    {
        geometry->allocate(static_cast<int>(points.size()));
    }
    QSGGeometry::Point2D* vertices = geometry->vertexDataAsPoint2D();
    for (std::size_t i = 0; i < points.size(); ++i)
    {
        vertices[i].set(static_cast<float>(points[i].x()), static_cast<float>(points[i].y()));
    }
    node->markDirty(QSGNode::DirtyGeometry);
}

/**
 * @brief Create an empty triangle node with a separate color for each vertex.
 * Colors must be premultiplied by alpha. The node owns its geometry and material.
 */
QSGGeometryNode* createColoredNode()
{
    auto* node = new QSGGeometryNode;
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    node->setGeometry(geometry);
    node->setFlag(QSGNode::OwnsGeometry);
    node->setMaterial(new QSGVertexColorMaterial);
    node->setFlag(QSGNode::OwnsMaterial);
    return node;
}

/// Append two rectangle triangles, assigning the supplied premultiplied color to all six vertices.
void addColoredRect(std::vector<QSGGeometry::ColoredPoint2D>& triangles, double x0, double y0, double x1, double y1, QRgb color)
{
    const auto r = static_cast<uchar>(qRed(color));
    const auto g = static_cast<uchar>(qGreen(color));
    const auto b = static_cast<uchar>(qBlue(color));
    const auto a = static_cast<uchar>(qAlpha(color));
    const auto fx0 = static_cast<float>(x0);
    const auto fy0 = static_cast<float>(y0);
    const auto fx1 = static_cast<float>(x1);
    const auto fy1 = static_cast<float>(y1);
    QSGGeometry::ColoredPoint2D p[6];
    p[0].set(fx0, fy0, r, g, b, a);
    p[1].set(fx1, fy0, r, g, b, a);
    p[2].set(fx0, fy1, r, g, b, a);
    p[3].set(fx0, fy1, r, g, b, a);
    p[4].set(fx1, fy0, r, g, b, a);
    p[5].set(fx1, fy1, r, g, b, a);
    triangles.insert(triangles.end(), std::begin(p), std::end(p));
}

/// Replace colored vertices, resize the buffer if needed, and mark the geometry dirty.
void setColoredPoints(QSGGeometryNode* node, const std::vector<QSGGeometry::ColoredPoint2D>& vertices)
{
    QSGGeometry* geometry = node->geometry();
    if (geometry->vertexCount() != static_cast<int>(vertices.size()))
    {
        geometry->allocate(static_cast<int>(vertices.size()));
    }
    std::copy(vertices.begin(), vertices.end(), geometry->vertexDataAsColoredPoint2D());
    node->markDirty(QSGNode::DirtyGeometry);
}

/// Root node of the background layer, behind the curves.
class BackgroundNode : public QSGNode
{
  public:
        /// Create persistent child nodes in drawing order: range shading, then grid lines.
    BackgroundNode()
    {
        shade = createFlatNode(QSGGeometry::DrawTriangles);
        grid = createFlatNode(QSGGeometry::DrawLines);
        appendChildNode(shade);
        appendChildNode(grid);
    }

    QSGGeometryNode* shade;
    QSGGeometryNode* grid;
};

/// Root node of the overlay layer, in front of the curves.
class OverlayNode : public QSGNode
{
  public:
        /// Create persistent child nodes in drawing order: bands, playhead, then intersection dots.
    OverlayNode()
    {
        bands = createColoredNode();
        playhead = createFlatNode(QSGGeometry::DrawTriangles);
        dots = createColoredNode();
        appendChildNode(bands);
        appendChildNode(playhead);
        appendChildNode(dots);
    }

    QSGGeometryNode* bands;
    QSGGeometryNode* playhead;
    QSGGeometryNode* dots;
};

/// Multiply RGB components by alpha while preserving alpha, as required by vertex-color blending.
QColor premultiplied(const QColor& c)
{
    const float a = c.alphaF();
    return QColor::fromRgbF(c.redF() * a, c.greenF() * a, c.blueF() * a, a);
}

/**
 * @brief Compute the distance from a point to the nearest point on a line segment.
 * Project onto the line and clamp the projection to the segment endpoints.
 * A zero-length segment is treated as a single point.
 */
double distanceToSegment(double px, double py, double ax, double ay, double bx, double by)
{
    const double dx = bx - ax;
    const double dy = by - ay;
    const double len2 = dx * dx + dy * dy;
    double t = 0.0;
    if (len2 > 0.0)
    {
        t = std::clamp(((px - ax) * dx + (py - ay) * dy) / len2, 0.0, 1.0);
    }
    return std::hypot(px - (ax + t * dx), py - (ay + t * dy));
}

}  // namespace

/// Child item drawing a layer of the viewer with regular scene graph nodes.
class CurvePaintLayer : public QQuickItem
{
  public:
    using UpdateNode = QSGNode* (CurveViewer::*)(QSGNode*);

    /// Parent the layer to the viewer and store the callback that builds its scene graph.
    CurvePaintLayer(CurveViewer* viewer, UpdateNode updateNode)
      : QQuickItem(viewer),
        _viewer(viewer),
        _updateNode(updateNode)
    {
        setFlag(QQuickItem::ItemHasContents, true);
    }

  protected:
        /// Forward Qt's scene graph update to the viewer callback, allowing reuse of oldNode.
        /// Qt invokes this during rendering synchronization, with the GUI thread blocked.
    QSGNode* updatePaintNode(QSGNode* oldNode, [[maybe_unused]] QQuickItem::UpdatePaintNodeData* data) override
    {
        return (_viewer->*_updateNode)(oldNode);
    }

  private:
    CurveViewer* _viewer;
    UpdateNode _updateNode;
};

/**
 * @brief Initialize axis models and create the background, curve and overlay layers.
 * Child creation order determines stacking. Appearance and band changes schedule
 * only the layers whose contents depend on them.
 */
CurveViewer::CurveViewer(QQuickItem* parent)
  : QQuickItem(parent),
    _xTicks(new CurveAxisTicks(this)),
    _yTicks(new CurveAxisTicks(this)),
    // Created back to front
    _backgroundLayer(new CurvePaintLayer(this, &CurveViewer::updateBackgroundNode)),
    _curvesLayer(new CurveLayer(this)),
    _overlayLayer(new CurvePaintLayer(this, &CurveViewer::updateOverlayNode))
{
    connect(this, &CurveViewer::appearanceChanged, this, [this]() { updateLayers(BackgroundLayer | OverlayLayer); });
    connect(this, &CurveViewer::bandsChanged, this, [this]() { updateLayers(OverlayLayer); });
}

/// Schedule asynchronous repaints for the layers selected by the Layer bitmask.
void CurveViewer::updateLayers(int layers)
{
    if (layers & BackgroundLayer)
    {
        _backgroundLayer->update();
    }
    if (layers & CurvesLayer)
    {
        _curvesLayer->update();
    }
    if (layers & OverlayLayer)
    {
        _overlayLayer->update();
    }
}

/**
 * @brief Replace the band model and reconnect its structural and data-change signals.
 * A null model removes the bands. bandsChanged() also schedules an overlay repaint.
 * Assigning the existing model is a no-op.
 */
void CurveViewer::setBands(CurveBandModel* bands)
{
    if (_bands == bands)
    {
        return;
    }

    if (_bands)
    {
        disconnect(_bands, nullptr, this, nullptr);
    }

    _bands = bands;
    if (_bands)
    {
        connect(_bands, &CurveBandModel::rowsInserted, this, &CurveViewer::bandsChanged);
        connect(_bands, &CurveBandModel::rowsRemoved, this, &CurveViewer::bandsChanged);
        connect(_bands, &CurveBandModel::modelReset, this, &CurveViewer::bandsChanged);
        connect(_bands, &CurveBandModel::dataChanged, this, &CurveViewer::bandsChanged);
    }
    Q_EMIT bandsChanged();
}

/// Return the height occupied by visible bands, including spacing after each visible band.
double CurveViewer::bandsHeight() const
{
    int visibleCount = 0;
    for (int row = 0; _bands && row < _bands->rowCount(); ++row)
    {
        visibleCount += _bands->band(row).visible ? 1 : 0;
    }
    return visibleCount * (_bandHeight + _bandSpacing);
}

/**
 * @brief Find the model row of the visible band containing item-coordinate py.
 * Hidden bands consume no space; spacing gaps and positions outside bands return -1.
 * Each band's top edge is included and its bottom edge is excluded.
 */
int CurveViewer::bandAt(double py) const
{
    double y0 = 0.0;
    for (int row = 0; _bands && row < _bands->rowCount(); ++row)
    {
        if (!_bands->band(row).visible)
        {
            continue;
        }
        if (py >= y0 && py < y0 + _bandHeight)
        {
            return row;
        }
        y0 += _bandHeight + _bandSpacing;
    }
    return -1;
}

/// Return a visible band's top pixel coordinate, or -1 for a missing, invalid or hidden band.
double CurveViewer::bandY(int row) const
{
    if (!_bands || !_bands->isValidRow(row) || !_bands->band(row).visible)
    {
        return -1.0;
    }
    double y0 = 0.0;
    for (int r = 0; r < row; ++r)
    {
        y0 += _bands->band(r).visible ? _bandHeight + _bandSpacing : 0.0;
    }
    return y0;
}

/**
 * @brief Replace the curve model, reconnect notifications and clear the current curve.
 * Refresh playhead values and curve/overlay rendering, then emit modelChanged().
 * A model reset also clears selection; assigning the existing model does nothing.
 */
void CurveViewer::setModel(CurveModel* model)
{
    if (_model == model)
    {
        return;
    }

    if (_model)
    {
        disconnect(_model, nullptr, this, nullptr);
    }

    _model = model;
    if (_model)
    {
        connect(_model, &CurveModel::rowsInserted, this, &CurveViewer::onModelRowsInserted);
        connect(_model, &CurveModel::rowsRemoved, this, &CurveViewer::onModelRowsRemoved);
        connect(_model, &CurveModel::modelReset, this, [this]() {
            setCurrentIndex(-1);
            onModelChanged();
        });
        connect(_model, &CurveModel::dataChanged, this, &CurveViewer::onModelChanged);
    }

    setCurrentIndex(-1);
    onModelChanged();
    Q_EMIT modelChanged();
}

/// Shift the current row past inserted rows to preserve selection, then refresh model-dependent content.
void CurveViewer::onModelRowsInserted(const QModelIndex&, int first, int last)
{
    if (_currentIndex >= first)
    {
        setCurrentIndex(_currentIndex + (last - first + 1));
    }
    onModelChanged();
}

/// Clear a removed selection or shift it past deleted rows, then refresh model-dependent content.
void CurveViewer::onModelRowsRemoved(const QModelIndex&, int first, int last)
{
    if (_currentIndex >= first && _currentIndex <= last)
    {
        setCurrentIndex(-1);
    }
    else if (_currentIndex > last)
    {
        setCurrentIndex(_currentIndex - (last - first + 1));
    }
    onModelChanged();
}

/// Recompute playhead samples and schedule curve and intersection-dot redraws after model changes.
void CurveViewer::onModelChanged()
{
    updatePlayheadValues();
    // Curves, and their intersections with the playhead
    updateLayers(CurvesLayer | OverlayLayer);
}

/// Set a finite playhead X value, notify bindings, resample curves and repaint only the overlay.
/// Unchanged or non-finite values are ignored.
void CurveViewer::setPlayhead(double x)
{
    if (x == _playhead || !std::isfinite(x))
    {
        return;
    }
    _playhead = x;
    Q_EMIT playheadChanged();
    updatePlayheadValues();
    updateLayers(OverlayLayer);
}

/**
 * @brief Sample every model row at the playhead, including hidden curves.
 * Preserve model-row order in the exposed list and notify bindings after replacement.
 * With no model the list is empty; valueAt() supplies NaN for unavailable samples.
 */
void CurveViewer::updatePlayheadValues()
{
    QVariantList values;
    if (_model)
    {
        const int n = _model->rowCount();
        values.reserve(n);
        for (int row = 0; row < n; ++row)
        {
            values.append(_model->curve(row).valueAt(_playhead));
        }
    }
    _playheadValues = std::move(values);
    Q_EMIT playheadValuesChanged();
}

/// Toggle the fitting interval and its background shading; notify and repaint only on a change.
void CurveViewer::setRangeEnabled(bool enabled)
{
    if (enabled == _rangeEnabled)
    {
        return;
    }
    _rangeEnabled = enabled;
    Q_EMIT rangeChanged();
    updateLayers(BackgroundLayer);
}

/// Store finite interval endpoints in ascending order, enable the range, and refresh shading.
/// The interval also restricts subsequent fit operations; it does not change the current view.
void CurveViewer::setRange(double start, double end)
{
    if (!std::isfinite(start) || !std::isfinite(end))
    {
        return;
    }
    _rangeStart = std::min(start, end);
    _rangeEnd = std::max(start, end);
    _rangeEnabled = true;
    Q_EMIT rangeChanged();
    updateLayers(BackgroundLayer);
}

/// Select a valid curve row, or clear selection with -1; repaint curves when selection changes.
/// The selected curve is emphasized during rendering, without changing the view or playhead.
void CurveViewer::setCurrentIndex(int row)
{
    if (row < 0 || !_model || !_model->isValidRow(row))
    {
        row = -1;
    }
    if (row == _currentIndex)
    {
        return;
    }
    _currentIndex = row;
    Q_EMIT currentIndexChanged();
    updateLayers(CurvesLayer);
}

/**
 * @brief Set the data rectangle mapped to the item's full width and height.
 * Reject non-finite bounds, reversed/too-small spans, and spans above maxViewSpan.
 * The minimum span grows with coordinate magnitude to avoid precision loss.
 * A changed view refreshes ticks, notifies bindings and schedules all three layers.
 */
void CurveViewer::setView(double xMin, double xMax, double yMin, double yMax)
{
    for (const double v : {xMin, xMax, yMin, yMax})
    {
        if (!std::isfinite(v))
        {
            return;
        }
    }
    const double spanX = xMax - xMin;
    const double spanY = yMax - yMin;
    // The span must stay well above the double precision of the coordinates
    const double minSpanX = std::max(minViewSpan, std::max(std::abs(xMin), std::abs(xMax)) * minRelativeViewSpan);
    const double minSpanY = std::max(minViewSpan, std::max(std::abs(yMin), std::abs(yMax)) * minRelativeViewSpan);
    if (spanX < minSpanX || spanY < minSpanY || spanX > maxViewSpan || spanY > maxViewSpan)
    {
        return;
    }
    if (xMin == _xMin && xMax == _xMax && yMin == _yMin && yMax == _yMax)
    {
        return;
    }

    _xMin = xMin;
    _xMax = xMax;
    _yMin = yMin;
    _yMax = yMax;
    updateTicks();
    Q_EMIT viewChanged();
    updateLayers(AllLayers);
}

/// Return the horizontal scale in item pixels per data unit.
double CurveViewer::scaleX() const { return width() / (_xMax - _xMin); }
/// Return the positive vertical scale in item pixels per data unit; mapping applies Y inversion.
double CurveViewer::scaleY() const { return height() / (_yMax - _yMin); }

/// Map data X to item pixels, with xMin at the left edge; values outside the view are not clamped.
double CurveViewer::xToPixel(double x) const { return (x - _xMin) * scaleX(); }
/// Map item pixels to data X, falling back to xMin when the item has no positive width.
double CurveViewer::pixelToX(double px) const { return width() > 0.0 ? _xMin + px / scaleX() : _xMin; }
/// Map data Y to item pixels, with yMax at the top and yMin at the bottom; do not clamp.
double CurveViewer::yToPixel(double y) const { return height() - (y - _yMin) * scaleY(); }
/// Invert the Y mapping, falling back to yMin when the item has no positive height.
double CurveViewer::pixelToY(double py) const { return height() > 0.0 ? _yMin + (height() - py) / scaleY() : _yMin; }

/// Translate the view by a pixel displacement so rendered content follows the cursor.
/// Convert through axis scales and reverse the vertical sign because item Y points down.
void CurveViewer::pan(double dx, double dy)
{
    if (width() <= 0.0 || height() <= 0.0)
    {
        return;
    }
    const double ox = dx / scaleX();
    const double oy = dy / scaleY();
    setView(_xMin - ox, _xMax - ox, _yMin + oy, _yMax + oy);
}

/**
 * @brief Scale each axis around an anchor expressed in item pixels.
 * Factors above one zoom in; factors below one zoom out. The anchor's data
 * coordinates stay at the same pixel position. Non-positive factors or item sizes are ignored;
 * setView() validates the resulting bounds.
 */
void CurveViewer::zoom(double factorX, double factorY, double anchorX, double anchorY)
{
    if (width() <= 0.0 || height() <= 0.0 || factorX <= 0.0 || factorY <= 0.0)
    {
        return;
    }
    const double ax = pixelToX(anchorX);
    const double ay = pixelToY(anchorY);
    setView(ax - (ax - _xMin) / factorX, ax + (_xMax - ax) / factorX, ay - (ay - _yMin) / factorY, ay + (_yMax - ay) / factorY);
}

/**
 * @brief Compute data bounds for a fit, restricted to the enabled X interval.
 * A non-negative row requests that curve even if hidden; a negative row combines
 * all visible curves. A missing model or invalid requested row yields invalid bounds.
 */
Bounds CurveViewer::fitSourceBounds(int row) const
{
    Bounds b;
    if (!_model)
    {
        return b;
    }

    double x0 = -std::numeric_limits<double>::infinity();
    double x1 = std::numeric_limits<double>::infinity();
    if (_rangeEnabled)
    {
        x0 = std::min(_rangeStart, _rangeEnd);
        x1 = std::max(_rangeStart, _rangeEnd);
    }

    if (row >= 0)
    {
        if (_model->isValidRow(row))
        {
            b = _model->curve(row).boundsInRange(x0, x1);
        }
        return b;
    }

    for (int r = 0; r < _model->rowCount(); ++r)
    {
        const Curve& c = _model->curve(r);
        if (c.visible)
        {
            b.extend(c.boundsInRange(x0, x1));
        }
    }
    return b;
}

/**
 * @brief Fit the requested axes to valid bounds while leaving other axes unchanged.
 * Add a 5% margin and expand flat extents to a usable span. For Y fitting, reserve
 * space above the curves for bands when at least 25% of the item remains available.
 * Apply the result through setView(), including its validity checks and notifications.
 */
void CurveViewer::fitBounds(const Bounds& b, bool fitX, bool fitY)
{
    if (!b.valid)
    {
        return;
    }

    // Expand a flat extent before adding symmetric margins to the requested axis bounds.
    const auto withMargin = [](double lo, double hi, double& outLo, double& outHi) {
        double span = hi - lo;
        if (span <= minViewSpan)
        {
            // Flat extent: center it with an arbitrary span
            span = std::max(1.0, std::abs(lo) * 0.1);
            lo -= 0.5 * span;
            hi += 0.5 * span;
        }
        outLo = lo - fitMargin * span;
        outHi = hi + fitMargin * span;
    };

    double x0 = _xMin, x1 = _xMax, y0 = _yMin, y1 = _yMax;
    if (fitX)
    {
        withMargin(b.xMin, b.xMax, x0, x1);
    }
    if (fitY)
    {
        withMargin(b.yMin, b.yMax, y0, y1);
        // Keep the curves below the bands, unless the bands take most of the view
        const double available = height() - bandsHeight();
        if (height() > 0.0 && available >= 0.25 * height())
        {
            y1 += (y1 - y0) * (height() - available) / available;
        }
    }
    setView(x0, x1, y0, y1);
}

/// Fit both axes to all visible curves, honoring the enabled X interval and band headroom.
void CurveViewer::fitAll() { fitBounds(fitSourceBounds(-1), true, true); }

/// Fit both axes to a specific curve; negative or invalid rows have no effect.
void CurveViewer::fitCurve(int row)
{
    if (row >= 0)
    {
        fitBounds(fitSourceBounds(row), true, true);
    }
}

/// Fit only the X axis to the requested curve's range-restricted bounds, preserving Y.
void CurveViewer::fitCurveHorizontally(int row)
{
    if (row >= 0)
    {
        fitBounds(fitSourceBounds(row), true, false);
    }
}

/// Fit only the Y axis to the requested curve's range-restricted bounds, preserving X.
void CurveViewer::fitCurveVertically(int row)
{
    if (row >= 0)
    {
        fitBounds(fitSourceBounds(row), false, true);
    }
}

/**
 * @brief Pick the visible curve nearest to an item-coordinate point within tolerance pixels.
 * Binary searches narrow the candidate samples to the horizontal picking interval;
 * distances are then measured against segments in pixel space, including isolated points.
 * Return -1 if nothing qualifies. Equal-distance ties favor the later model row.
 */
int CurveViewer::curveAt(double px, double py, double tolerance) const
{
    if (!_model || width() <= 0.0 || height() <= 0.0)
    {
        return -1;
    }

    const double x0 = pixelToX(px - tolerance);
    const double x1 = pixelToX(px + tolerance);

    int bestRow = -1;
    double bestDistance = tolerance;
    for (int row = 0; row < _model->rowCount(); ++row)
    {
        const Curve& c = _model->curve(row);
        if (!c.visible || c.x.empty())
        {
            continue;
        }

        // Candidate segments overlap [x0, x1]
        auto first = std::lower_bound(c.x.begin(), c.x.end(), x0);
        if (first != c.x.begin())
        {
            --first;
        }
        const auto last = std::upper_bound(first, c.x.end(), x1);
        const std::size_t i0 = static_cast<std::size_t>(first - c.x.begin());
        const std::size_t i1 = std::min(static_cast<std::size_t>(last - c.x.begin()), c.x.size() - 1);

        for (std::size_t i = i0; i <= i1; ++i)
        {
            const std::size_t j = std::min(i + 1, i1);
            const double distance = distanceToSegment(px, py, xToPixel(c.x[i]), yToPixel(c.y[i]), xToPixel(c.x[j]), yToPixel(c.y[j]));
            if (distance <= bestDistance)
            {
                bestDistance = distance;
                bestRow = row;
            }
        }
    }
    return bestRow;
}

/// On item resize, resize all child layers, recompute pixel-based ticks and repaint every layer.
/// Position-only geometry changes require no additional work beyond the base implementation.
void CurveViewer::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
    {
        for (QQuickItem* layer : {static_cast<QQuickItem*>(_backgroundLayer), static_cast<QQuickItem*>(_curvesLayer), static_cast<QQuickItem*>(_overlayLayer)})
        {
            layer->setSize(newGeometry.size());
        }
        updateTicks();
        updateLayers(AllLayers);
    }
}

/// Recompute tick models with at least 80 pixels of X spacing and 40 pixels of inverted Y spacing.
void CurveViewer::updateTicks()
{
    // Minimal spacing between graduations, in pixels
    _xTicks->update(_xMin, _xMax, width(), 80.0, false);
    _yTicks->update(_yMin, _yMax, height(), 40.0, true);
}

/**
 * @brief Create or reuse the background scene graph during rendering synchronization.
 * Build shading rectangles outside the enabled X interval, clipped to the item width,
 * and grid lines at tick positions aligned to pixel centers for crisp one-pixel lines.
 * Update existing child geometry/materials and return the root for Qt to retain.
 */
QSGNode* CurveViewer::updateBackgroundNode(QSGNode* oldNode)
{
    auto* root = static_cast<BackgroundNode*>(oldNode);
    if (!root)
    {
        root = new BackgroundNode;
    }

    const double w = width();
    const double h = height();

    // Range shading, outside of the X interval
    {
        std::vector<QPointF> triangles;
        if (_rangeEnabled)
        {
            const double r0 = std::clamp(xToPixel(std::min(_rangeStart, _rangeEnd)), 0.0, w);
            const double r1 = std::clamp(xToPixel(std::max(_rangeStart, _rangeEnd)), 0.0, w);
            if (r0 > 0.0)
            {
                addRect(triangles, 0.0, 0.0, r0, h);
            }
            if (r1 < w)
            {
                addRect(triangles, r1, 0.0, w, h);
            }
        }
        setPoints(root->shade, triangles);
        setFlatColor(root->shade, _rangeShadeColor);
    }

    // Grid, aligned on pixel centers
    {
        std::vector<QPointF> lines;
        for (const auto& t : _xTicks->ticks())
        {
            const double px = std::floor(t.position) + 0.5;
            lines.insert(lines.end(), {{px, 0.0}, {px, h}});
        }
        for (const auto& t : _yTicks->ticks())
        {
            const double py = std::floor(t.position) + 0.5;
            lines.insert(lines.end(), {{0.0, py}, {w, py}});
        }
        setPoints(root->grid, lines);
        setFlatColor(root->grid, _gridColor);
    }

    return root;
}

/**
 * @brief Synchronize curve resources and ordered draw commands with the renderer.
 * Called on the render thread while the GUI thread is blocked. Register model curves,
 * retain only their IDs, and pass view transforms without rebuilding samples for navigation.
 * Draw envelopes before lines, dim non-current curves, then redraw a visible current
 * curve with a thicker line on top. Line widths account for the device pixel ratio.
 */
void CurveViewer::synchronizeCurves(CurveRenderer& renderer) const
{
    const double h = height();
    const double dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const double sx = scaleX();
    const double sy = scaleY();

    // The current curve is drawn again on top of the others
    renderer.clearDraws();

    const int rowCount = _model ? _model->rowCount() : 0;
    const bool hasCurrent = _model && _model->isValidRow(_currentIndex) && _model->curve(_currentIndex).visible;
    // Keep the current curve's alpha; dim every other curve whenever a current row is set.
    const auto curveColor = [this](int row) {
        QColor color = _model->curve(row).color;
        if (_currentIndex >= 0 && row != _currentIndex)
        {
            color.setAlphaF(static_cast<float>(color.alphaF() * inactiveOpacity));
        }
        return color;
    };

    // Envelopes behind all the lines, the current curve ones on top of the others
    // Submit each envelope with its own opacity multiplied by the curve's effective alpha.
    const auto addFills = [&](int row) {
        const Curve& c = _model->curve(row);
        for (std::size_t e = 0; e < c.envelopes.size(); ++e)
        {
            QColor color = curveColor(row);
            color.setAlphaF(color.alphaF() * c.envelopes[e].opacity);
            renderer.addFillDraw(c.id, static_cast<int>(e), color, sx, sy, _xMin, _yMin, h);
        }
    };
    std::unordered_set<quint64> ids;
    for (int row = 0; row < rowCount; ++row)
    {
        const Curve& c = _model->curve(row);
        ids.insert(c.id);
        renderer.addCurve(c);
        if (c.visible && !(hasCurrent && row == _currentIndex))
        {
            addFills(row);
        }
    }
    if (hasCurrent)
    {
        addFills(_currentIndex);
    }

    for (int row = 0; row < rowCount; ++row)
    {
        const Curve& c = _model->curve(row);
        if (c.visible)
        {
            renderer.addDraw(c.id, curveColor(row), static_cast<float>(lineWidth * dpr), sx, sy, _xMin, _yMin, h);
        }
    }
    renderer.retainCurves(ids);

    if (hasCurrent)
    {
        const Curve& c = _model->curve(_currentIndex);
        renderer.addDraw(c.id, c.color, static_cast<float>(currentLineWidth * dpr), sx, sy, _xMin, _yMin, h);
    }
}

/**
 * @brief Create or reuse the foreground scene graph during rendering synchronization.
 * Stack visible color bands at the top, merge adjacent same-color cells, and skip
 * redundant sub-pixel cells to bound geometry by the visible pixel columns.
 * Draw a two-pixel-wide playhead, then two concentric 12-triangle disks at each
 * visible curve intersection: an outer playhead-color disk and an inner curve-color disk.
 * Colors use premultiplied alpha; existing child nodes are reused and empty geometry hides content.
 */
QSGNode* CurveViewer::updateOverlayNode(QSGNode* oldNode)
{
    auto* root = static_cast<OverlayNode*>(oldNode);
    if (!root)
    {
        root = new OverlayNode;
    }

    const double w = width();
    const double h = height();

    // Color bands, stacked from the top: at most one quad per pixel column, whatever the number of cells
    {
        std::vector<QSGGeometry::ColoredPoint2D> triangles;
        double y0 = 0.0;
        for (int row = 0; _bands && row < _bands->rowCount(); ++row)
        {
            const Band& b = _bands->band(row);
            if (!b.visible)
            {
                continue;
            }
            const double y1 = y0 + _bandHeight;
            const std::size_t n = b.x.size();
            // Binary-search cell right edges: return the first cell ending strictly after x, or n.
            const auto cellEndingAfter = [&b](double x) {
                return static_cast<std::size_t>(std::upper_bound(b.edges.begin() + 1, b.edges.end(), x) - (b.edges.begin() + 1));
            };

            // Adjacent cells of the same color are merged in a single run
            double runX0 = 0.0;
            double runX1 = 0.0;
            QRgb runColor = 0;
            // Emit a completed color run only when it has non-zero alpha and positive width.
            const auto flush = [&]() {
                if (qAlpha(runColor) != 0 && runX1 > runX0)
                {
                    addColoredRect(triangles, runX0, y0, runX1, y1, runColor);
                }
            };

            std::size_t i = n > 0 ? cellEndingAfter(_xMin) : 0;
            while (i < n && b.edges[i] < _xMax)
            {
                double px0 = std::max(xToPixel(b.edges[i]), runX1);
                double px1 = std::min(xToPixel(b.edges[i + 1]), w);
                std::size_t next = i + 1;
                if (px1 - px0 < 1.0)
                {
                    // Sub-pixel cell: it fills the rest of its pixel column, the other cells of the column are skipped
                    px1 = std::min(std::floor(px0) + 1.0, w);
                    next = std::max(next, cellEndingAfter(pixelToX(px1)));
                }

                const QRgb color = b.colors[i];
                if (color != runColor || px0 > runX1)
                {
                    flush();
                    runX0 = px0;
                    runColor = color;
                }
                runX1 = px1;
                i = next;
            }
            flush();
            y0 = y1 + _bandSpacing;
        }
        setColoredPoints(root->bands, triangles);
    }

    // Playhead
    const double playheadX = xToPixel(_playhead);
    {
        std::vector<QPointF> triangles;
        if (playheadX >= -1.0 && playheadX <= w + 1.0)
        {
            addRect(triangles, playheadX - 1.0, 0.0, playheadX + 1.0, h);
        }
        setPoints(root->playhead, triangles);
        setFlatColor(root->playhead, _playheadColor);
    }

    // Intersections of the playhead with the visible curves
    {
        constexpr int dotSegments = 12;
        constexpr float outerRadius = 4.5f;
        constexpr float innerRadius = 3.f;

        std::vector<QSGGeometry::ColoredPoint2D> vertices;
        // Approximate a filled disk by a triangle fan with a shared center and 12 boundary segments.
        const auto addDisk = [&vertices](float cx, float cy, float radius, const QColor& color) {
            const auto r = static_cast<uchar>(color.red());
            const auto g = static_cast<uchar>(color.green());
            const auto b = static_cast<uchar>(color.blue());
            const auto a = static_cast<uchar>(color.alpha());
            for (int i = 0; i < dotSegments; ++i)
            {
                const float a0 = static_cast<float>(2.0 * std::numbers::pi * i / dotSegments);
                const float a1 = static_cast<float>(2.0 * std::numbers::pi * (i + 1) / dotSegments);
                QSGGeometry::ColoredPoint2D p[3];
                p[0].set(cx, cy, r, g, b, a);
                p[1].set(cx + radius * std::cos(a0), cy + radius * std::sin(a0), r, g, b, a);
                p[2].set(cx + radius * std::cos(a1), cy + radius * std::sin(a1), r, g, b, a);
                vertices.insert(vertices.end(), std::begin(p), std::end(p));
            }
        };

        if (_model && playheadX >= 0.0 && playheadX <= w)
        {
            for (int row = 0; row < _model->rowCount(); ++row)
            {
                const Curve& c = _model->curve(row);
                const double v = c.valueAt(_playhead);
                if (!c.visible || !std::isfinite(v))
                {
                    continue;
                }
                const double py = yToPixel(v);
                if (py < -outerRadius || py > h + outerRadius)
                {
                    continue;
                }
                // Vertex colors are expected premultiplied
                addDisk(static_cast<float>(playheadX), static_cast<float>(py), outerRadius, premultiplied(_playheadColor));
                addDisk(static_cast<float>(playheadX), static_cast<float>(py), innerRadius, premultiplied(c.color));
            }
        }

        setColoredPoints(root->dots, vertices);
    }

    return root;
}

}  // namespace curveViewer
