#pragma once

#include "CurveAxisTicks.hpp"
#include "CurveBandModel.hpp"
#include "CurveModel.hpp"

#include <QColor>
#include <QPointer>
#include <QQuickItem>
#include <QVariantList>

namespace curveViewer {

class CurveLayer;
class CurvePaintLayer;
class CurveRenderer;

/**
 * @brief Draw the curves of a CurveModel.
 *
 * Draws, from back to front: the shading outside of the X interval, the grid,
 * the curves (current curve on top), the color bands stacked at the top,
 * the playhead and its intersections with the curves.
 *
 * The view is the data rectangle [xMin, xMax] x [yMin, yMax] mapped onto the item, Y pointing up.
 * Curve samples are uploaded once to the GPU (see CurveRenderer): panning and zooming only update uniforms.
 *
 * Drawing is split in three child items, each one updated only when its content changes:
 * the background (shading, grid), the curves (rendered into a texture, see CurveLayer),
 * and the overlay (bands, playhead). Moving the playhead does not render the curves again.
 *
 * Interactions are implemented in QML using the mapping and navigation methods of this class.
 */
class CurveViewer : public QQuickItem
{
    Q_OBJECT

    /// Curves to display
    Q_PROPERTY(curveViewer::CurveModel* model READ model WRITE setModel NOTIFY modelChanged)

    /// Color bands drawn at the top of the view, following the X axis
    Q_PROPERTY(curveViewer::CurveBandModel* bands READ bands WRITE setBands NOTIFY bandsChanged)
    /// Height of each band and spacing between bands, in pixels
    Q_PROPERTY(double bandHeight MEMBER _bandHeight NOTIFY bandsChanged)
    Q_PROPERTY(double bandSpacing MEMBER _bandSpacing NOTIFY bandsChanged)
    /// Total height of the visible bands, in pixels
    Q_PROPERTY(double bandsHeight READ bandsHeight NOTIFY bandsChanged)

    /// Displayed data rectangle
    Q_PROPERTY(double xMin READ xMin NOTIFY viewChanged)
    Q_PROPERTY(double xMax READ xMax NOTIFY viewChanged)
    Q_PROPERTY(double yMin READ yMin NOTIFY viewChanged)
    Q_PROPERTY(double yMax READ yMax NOTIFY viewChanged)

    /// X position of the playhead
    Q_PROPERTY(double playhead READ playhead WRITE setPlayhead NOTIFY playheadChanged)
    /// Value of each curve at the playhead position (NaN if outside of the curve), indexed by model row
    Q_PROPERTY(QVariantList playheadValues READ playheadValues NOTIFY playheadValuesChanged)

    /// X interval restricting the fit computations
    Q_PROPERTY(bool rangeEnabled READ rangeEnabled WRITE setRangeEnabled NOTIFY rangeChanged)
    Q_PROPERTY(double rangeStart READ rangeStart NOTIFY rangeChanged)
    Q_PROPERTY(double rangeEnd READ rangeEnd NOTIFY rangeChanged)

    /// Row of the current curve, drawn on top of the others, -1 if none
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

    /// Appearance
    Q_PROPERTY(QColor gridColor MEMBER _gridColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor playheadColor MEMBER _playheadColor NOTIFY appearanceChanged)
    Q_PROPERTY(QColor rangeShadeColor MEMBER _rangeShadeColor NOTIFY appearanceChanged)

    /// Graduations, positions are in item coordinates
    Q_PROPERTY(curveViewer::CurveAxisTicks* xTicks READ xTicks CONSTANT)
    Q_PROPERTY(curveViewer::CurveAxisTicks* yTicks READ yTicks CONSTANT)

  public:
    explicit CurveViewer(QQuickItem* parent = nullptr);

    CurveModel* model() const { return _model; }
    void setModel(CurveModel* model);

    CurveBandModel* bands() const { return _bands; }
    void setBands(CurveBandModel* bands);
    double bandsHeight() const;

    double xMin() const { return _xMin; }
    double xMax() const { return _xMax; }
    double yMin() const { return _yMin; }
    double yMax() const { return _yMax; }

    double playhead() const { return _playhead; }
    void setPlayhead(double x);
    QVariantList playheadValues() const { return _playheadValues; }

    bool rangeEnabled() const { return _rangeEnabled; }
    double rangeStart() const { return _rangeStart; }
    double rangeEnd() const { return _rangeEnd; }
    void setRangeEnabled(bool enabled);

    int currentIndex() const { return _currentIndex; }
    void setCurrentIndex(int row);

    CurveAxisTicks* xTicks() const { return _xTicks; }
    CurveAxisTicks* yTicks() const { return _yTicks; }

    /// Set the displayed data rectangle, ignored if degenerated.
    Q_INVOKABLE void setView(double xMin, double xMax, double yMin, double yMax);
    /// Set the X interval and enable it.
    Q_INVOKABLE void setRange(double start, double end);

    /// Mapping between data space and item coordinates
    Q_INVOKABLE double xToPixel(double x) const;
    Q_INVOKABLE double pixelToX(double px) const;
    Q_INVOKABLE double yToPixel(double y) const;
    Q_INVOKABLE double pixelToY(double py) const;

    /// Translate the view by a displacement in pixels (content follows the cursor).
    Q_INVOKABLE void pan(double dx, double dy);
    /// Zoom in (factor > 1) or out (factor < 1) around an anchor in item coordinates.
    Q_INVOKABLE void zoom(double factorX, double factorY, double anchorX, double anchorY);

    /// Fit all visible curves, restricted to the X interval if enabled.
    Q_INVOKABLE void fitAll();
    /// Fit a curve on both axes, restricted to the X interval if enabled.
    Q_INVOKABLE void fitCurve(int row);
    /// Fit the X extent of a curve, restricted to the X interval if enabled.
    Q_INVOKABLE void fitCurveHorizontally(int row);
    /// Fit the Y extent of a curve, computed on the samples within the X interval if enabled.
    Q_INVOKABLE void fitCurveVertically(int row);

    /// Row of the visible curve closest to (px, py) in item coordinates, -1 if none within tolerance pixels.
    Q_INVOKABLE int curveAt(double px, double py, double tolerance) const;

    /// Row of the visible band at py in item coordinates, -1 if none.
    Q_INVOKABLE int bandAt(double py) const;
    /// Top of a band in item coordinates, -1 if the band is hidden or invalid.
    Q_INVOKABLE double bandY(int row) const;

  Q_SIGNALS:
    void modelChanged();
    void bandsChanged();
    void viewChanged();
    void playheadChanged();
    void playheadValuesChanged();
    void rangeChanged();
    void currentIndexChanged();
    void appearanceChanged();

  protected:
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

  private:
    friend class CurvePaintLayer;
    friend class CurveRenderer;

    /// Child items drawing the viewer, back to front
    enum Layer
    {
        BackgroundLayer = 1,
        CurvesLayer = 2,
        OverlayLayer = 4,
        AllLayers = BackgroundLayer | CurvesLayer | OverlayLayer
    };
    /// Schedule the update of a combination of layers.
    void updateLayers(int layers);

    /// Range shading and grid.
    QSGNode* updateBackgroundNode(QSGNode* oldNode);
    /// Curve draws, called on the render thread while the GUI thread is blocked.
    void synchronizeCurves(CurveRenderer& renderer) const;
    /// Color bands, playhead and its intersections with the curves.
    QSGNode* updateOverlayNode(QSGNode* oldNode);

    void onModelRowsInserted(const QModelIndex& parent, int first, int last);
    void onModelRowsRemoved(const QModelIndex& parent, int first, int last);
    void onModelChanged();
    void updateTicks();
    void updatePlayheadValues();

    /// Fit bounds on the requested axes, adding a margin.
    void fitBounds(const Bounds& bounds, bool fitX, bool fitY);
    /// Bounds of a curve (or of all visible curves if row < 0) restricted to the X interval if enabled.
    Bounds fitSourceBounds(int row) const;

    double scaleX() const;
    double scaleY() const;

    QPointer<CurveModel> _model;
    QPointer<CurveBandModel> _bands;
    double _bandHeight = 12.0;
    double _bandSpacing = 1.0;

    double _xMin = 0.0;
    double _xMax = 100.0;
    double _yMin = -1.0;
    double _yMax = 1.0;

    double _playhead = 0.0;
    QVariantList _playheadValues;

    bool _rangeEnabled = false;
    double _rangeStart = 0.0;
    double _rangeEnd = 0.0;

    int _currentIndex = -1;

    QColor _gridColor = QColor(255, 255, 255, 20);
    QColor _playheadColor = QColor(42, 130, 218);
    QColor _rangeShadeColor = QColor(0, 0, 0, 80);

    CurveAxisTicks* _xTicks;
    CurveAxisTicks* _yTicks;

    CurvePaintLayer* _backgroundLayer;
    CurveLayer* _curvesLayer;
    CurvePaintLayer* _overlayLayer;
};

}  // namespace curveViewer
