#pragma once

#include <Core/LayerItem.hpp>
#include <GridLayer/GridRenderable.hpp>

#include <QtGlobal>

/**
 * @brief Layer that renders an infinite ground grid on the plane y = 0.
 *
 * The grid adapts to the zoom level: it draws three consecutive levels of
 * spacing baseSpacing * subdivisions^k chosen from the on-screen cell size,
 * cross-fading between them so that lines never become denser than
 * minCellPixels. Lines are anti-aliased with constant pixel width.
 */
class GridLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Finest grid spacing in world units. Coarser levels are baseSpacing * subdivisions^k. */
    Q_PROPERTY(float baseSpacing READ baseSpacing WRITE setBaseSpacing NOTIFY baseSpacingChanged)
    /** @brief Ratio between the spacings of two consecutive grid levels (at least 2). */
    Q_PROPERTY(float subdivisions READ subdivisions WRITE setSubdivisions NOTIFY subdivisionsChanged)
    /** @brief Width of major lines in pixels. */
    Q_PROPERTY(float majorLineWidth READ majorLineWidth WRITE setMajorLineWidth NOTIFY majorLineWidthChanged)
    /** @brief Width of minor lines in pixels. */
    Q_PROPERTY(float minorLineWidth READ minorLineWidth WRITE setMinorLineWidth NOTIFY minorLineWidthChanged)
    /** @brief Opacity of minor lines in [0, 1]; major lines are fully opaque. */
    Q_PROPERTY(float minorOpacity READ minorOpacity WRITE setMinorOpacity NOTIFY minorOpacityChanged)
    /** @brief Cell size in pixels below which a grid level is fully faded out. A level is fully visible once its cells reach minCellPixels * subdivisions pixels. */
    Q_PROPERTY(float minCellPixels READ minCellPixels WRITE setMinCellPixels NOTIFY minCellPixelsChanged)

  public:
    explicit GridLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    float baseSpacing() const
    {
        return _baseSpacing;
    }
    void setBaseSpacing(float value)
    {
        if (qFuzzyCompare(_baseSpacing, value))
        {
            return;
        }
        _baseSpacing = value;
        emit baseSpacingChanged();
        emit dataReady();
    }

    float subdivisions() const
    {
        return _subdivisions;
    }
    void setSubdivisions(float value)
    {
        if (qFuzzyCompare(_subdivisions, value))
        {
            return;
        }
        _subdivisions = value;
        emit subdivisionsChanged();
        emit dataReady();
    }

    float majorLineWidth() const
    {
        return _majorLineWidth;
    }
    void setMajorLineWidth(float value)
    {
        if (qFuzzyCompare(_majorLineWidth, value))
        {
            return;
        }
        _majorLineWidth = value;
        emit majorLineWidthChanged();
        emit dataReady();
    }

    float minorLineWidth() const
    {
        return _minorLineWidth;
    }
    void setMinorLineWidth(float value)
    {
        if (qFuzzyCompare(_minorLineWidth, value))
        {
            return;
        }
        _minorLineWidth = value;
        emit minorLineWidthChanged();
        emit dataReady();
    }

    float minorOpacity() const
    {
        return _minorOpacity;
    }
    void setMinorOpacity(float value)
    {
        if (qFuzzyCompare(_minorOpacity, value))
        {
            return;
        }
        _minorOpacity = value;
        emit minorOpacityChanged();
        emit dataReady();
    }

    float minCellPixels() const
    {
        return _minCellPixels;
    }
    void setMinCellPixels(float value)
    {
        if (qFuzzyCompare(_minCellPixels, value))
        {
            return;
        }
        _minCellPixels = value;
        emit minCellPixelsChanged();
        emit dataReady();
    }

    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<GridRenderable>();
    }

    bool rendersInBackground() const override
    {
        return true;
    }

  signals:
    void baseSpacingChanged();
    void subdivisionsChanged();
    void majorLineWidthChanged();
    void minorLineWidthChanged();
    void minorOpacityChanged();
    void minCellPixelsChanged();

  private:
    float _baseSpacing = 0.1f;
    float _subdivisions = 10.0f;
    float _majorLineWidth = 1.25f;
    float _minorLineWidth = 1.0f;
    float _minorOpacity = 0.35f;
    float _minCellPixels = 2.0f;
};
