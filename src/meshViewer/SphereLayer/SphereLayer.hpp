#pragma once

#include <Core/LayerItem.hpp>
#include <SphereLayer/SphereRenderable.hpp>

#include <QVariantList>

/** @brief Layer that renders GPU-instanced spheres at each picked point. */
class SphereLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Picked point positions (QVector3D values) where spheres are drawn. */
    Q_PROPERTY(QVariantList positions READ positions WRITE setPositions NOTIFY positionsChanged)

    /**
     * @brief Sphere radius.
     * Expressed in world units, or in pixels when @ref fixedSize is true.
     */
    Q_PROPERTY(float size READ size WRITE setSize NOTIFY sizeChanged)

    /** @brief If true, spheres keep a constant screen size whatever their distance to the camera. */
    Q_PROPERTY(bool fixedSize READ fixedSize WRITE setFixedSize NOTIFY fixedSizeChanged)

  public:
    explicit SphereLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    QVariantList positions() const
    {
        QVariantList values;
        values.reserve(static_cast<qsizetype>(_positions.size()));

        for (const QVector3D& position : _positions)
        {
            values.push_back(QVariant::fromValue(position));
        }

        return values;
    }

    void setPositions(const QVariantList& positions)
    {
        std::vector<QVector3D> convertedPositions;
        convertedPositions.reserve(static_cast<size_t>(positions.size()));

        for (const QVariant& value : positions)
        {
            if (value.canConvert<QVector3D>())
            {
                convertedPositions.push_back(value.value<QVector3D>());
            }
        }

        _positions = std::move(convertedPositions);
        _positionsDirty = true;
        emit positionsChanged();
        emit dataReady();
    }

    float size() const
    {
        return _size;
    }

    void setSize(float size)
    {
        if (qFuzzyCompare(_size, size))
        {
            return;
        }

        _size = size;
        _paramsDirty = true;
        emit sizeChanged();
        emit dataReady();
    }

    bool fixedSize() const
    {
        return _fixedSize;
    }

    void setFixedSize(bool fixedSize)
    {
        if (_fixedSize == fixedSize)
        {
            return;
        }

        _fixedSize = fixedSize;
        _paramsDirty = true;
        emit fixedSizeChanged();
        emit dataReady();
    }

    bool paramsDirty() const
    {
        return _paramsDirty;
    }
    void clearParamsDirty()
    {
        _paramsDirty = false;
    }

    const std::vector<QVector3D>& renderPositions() const
    {
        return _positions;
    }

    bool positionsDirty() const
    {
        return _positionsDirty;
    }
    void clearPositionsDirty()
    {
        _positionsDirty = false;
    }

    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<SphereRenderable>();
    }

  signals:
    void positionsChanged();
    void sizeChanged();
    void fixedSizeChanged();

  private:
    std::vector<QVector3D> _positions;
    bool _positionsDirty = false;
    float _size = 20.0f;
    bool _fixedSize = true;
    bool _paramsDirty = true;
};
