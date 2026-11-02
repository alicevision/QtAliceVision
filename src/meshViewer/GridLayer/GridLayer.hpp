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
    /**
     * @brief Build the layer with default grid settings.
     * @param[in] parent Optional Qt parent object.
     */
    explicit GridLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    /** @brief Get the finest grid spacing, in world units. */
    float baseSpacing() const
    {
        return _baseSpacing;
    }
    /**
     * @brief Set the finest grid spacing.
     * @param[in] value Spacing, in world units.
     */
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

    /** @brief Get the ratio between two consecutive grid levels. */
    float subdivisions() const
    {
        return _subdivisions;
    }
    /**
     * @brief Set the ratio between two consecutive grid levels.
     * @param[in] value Ratio (at least 2).
     */
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

    /** @brief Get the width of major lines, in pixels. */
    float majorLineWidth() const
    {
        return _majorLineWidth;
    }
    /**
     * @brief Set the width of major lines.
     * @param[in] value Width, in pixels.
     */
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

    /** @brief Get the width of minor lines, in pixels. */
    float minorLineWidth() const
    {
        return _minorLineWidth;
    }
    /**
     * @brief Set the width of minor lines.
     * @param[in] value Width, in pixels.
     */
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

    /** @brief Get the opacity of minor lines. */
    float minorOpacity() const
    {
        return _minorOpacity;
    }
    /**
     * @brief Set the opacity of minor lines.
     * @param[in] value Opacity, in [0, 1].
     */
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

    /** @brief Get the cell size below which a level is fully faded out, in pixels. */
    float minCellPixels() const
    {
        return _minCellPixels;
    }
    /**
     * @brief Set the cell size below which a level is fully faded out.
     * @param[in] value Cell size, in pixels.
     */
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

    /** @brief Create a GridRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<GridRenderable>();
    }

    /** @brief The grid is drawn before all other layers. */
    bool rendersInBackground() const override
    {
        return true;
    }

  signals:
    /** @brief Emitted when the base spacing changes. */
    void baseSpacingChanged();
    /** @brief Emitted when the subdivision ratio changes. */
    void subdivisionsChanged();
    /** @brief Emitted when the major line width changes. */
    void majorLineWidthChanged();
    /** @brief Emitted when the minor line width changes. */
    void minorLineWidthChanged();
    /** @brief Emitted when the minor line opacity changes. */
    void minorOpacityChanged();
    /** @brief Emitted when the minimum cell size changes. */
    void minCellPixelsChanged();

  private:
    /** @brief Finest grid spacing, in world units. */
    float _baseSpacing = 0.1f;
    /** @brief Ratio between two consecutive grid levels. */
    float _subdivisions = 10.0f;
    /** @brief Major line width, in pixels. */
    float _majorLineWidth = 1.25f;
    /** @brief Minor line width, in pixels. */
    float _minorLineWidth = 1.0f;
    /** @brief Minor line opacity. */
    float _minorOpacity = 0.35f;
    /** @brief Cell size below which a level is fully faded out, in pixels. */
    float _minCellPixels = 2.0f;
};
