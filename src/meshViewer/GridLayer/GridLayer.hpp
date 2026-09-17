#pragma once

#include <Core/LayerItem.hpp>
#include <GridLayer/GridRenderable.hpp>

#include <QtGlobal>

/** @brief Layer that renders a fullscreen quad shaded by a custom fragment shader. */
class GridLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float majorSpacing READ majorSpacing WRITE setMajorSpacing NOTIFY majorSpacingChanged)
    Q_PROPERTY(float minorSpacing READ minorSpacing WRITE setMinorSpacing NOTIFY minorSpacingChanged)
    Q_PROPERTY(float majorLineWidth READ majorLineWidth WRITE setMajorLineWidth NOTIFY majorLineWidthChanged)
    Q_PROPERTY(float minorLineWidth READ minorLineWidth WRITE setMinorLineWidth NOTIFY minorLineWidthChanged)
    Q_PROPERTY(float minorOpacity READ minorOpacity WRITE setMinorOpacity NOTIFY minorOpacityChanged)
    Q_PROPERTY(float minorFadeStartPixels READ minorFadeStartPixels WRITE setMinorFadeStartPixels NOTIFY minorFadeStartPixelsChanged)
    Q_PROPERTY(float minorFadeEndPixels READ minorFadeEndPixels WRITE setMinorFadeEndPixels NOTIFY minorFadeEndPixelsChanged)

  public:
    explicit GridLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    float majorSpacing() const
    {
        return _majorSpacing;
    }
    void setMajorSpacing(float value)
    {
        if (qFuzzyCompare(_majorSpacing, value))
            return;
        _majorSpacing = value;
        emit majorSpacingChanged();
        emit dataReady();
    }

    float minorSpacing() const
    {
        return _minorSpacing;
    }
    void setMinorSpacing(float value)
    {
        if (qFuzzyCompare(_minorSpacing, value))
            return;
        _minorSpacing = value;
        emit minorSpacingChanged();
        emit dataReady();
    }

    float majorLineWidth() const
    {
        return _majorLineWidth;
    }
    void setMajorLineWidth(float value)
    {
        if (qFuzzyCompare(_majorLineWidth, value))
            return;
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
            return;
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
            return;
        _minorOpacity = value;
        emit minorOpacityChanged();
        emit dataReady();
    }

    float minorFadeStartPixels() const
    {
        return _minorFadeStartPixels;
    }
    void setMinorFadeStartPixels(float value)
    {
        if (qFuzzyCompare(_minorFadeStartPixels, value))
            return;
        _minorFadeStartPixels = value;
        emit minorFadeStartPixelsChanged();
        emit dataReady();
    }

    float minorFadeEndPixels() const
    {
        return _minorFadeEndPixels;
    }
    void setMinorFadeEndPixels(float value)
    {
        if (qFuzzyCompare(_minorFadeEndPixels, value))
            return;
        _minorFadeEndPixels = value;
        emit minorFadeEndPixelsChanged();
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
    void majorSpacingChanged();
    void minorSpacingChanged();
    void majorLineWidthChanged();
    void minorLineWidthChanged();
    void minorOpacityChanged();
    void minorFadeStartPixelsChanged();
    void minorFadeEndPixelsChanged();

  private:
    float _majorSpacing = 1.0f;
    float _minorSpacing = 0.1f;
    float _majorLineWidth = 1.25f;
    float _minorLineWidth = 1.0f;
    float _minorOpacity = 0.35f;
    float _minorFadeStartPixels = 10.0f;
    float _minorFadeEndPixels = 18.0f;
};
