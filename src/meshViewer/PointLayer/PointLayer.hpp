#pragma once

#include <Core/LayerItem.hpp>
#include <PointLayer/PointRenderable.hpp>

#include <QVariantList>

/** @brief Layer that renders GPU-instanced spheres at each point. */
class PointLayer : public LayerItem
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
    /**
     * @brief Build an empty layer.
     * @param[in] parent Optional Qt parent object.
     */
    explicit PointLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    /** @brief Get the sphere centers as a list of QVector3D values. */
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

    /**
     * @brief Replace the sphere centers.
     * @param[in] positions List of QVector3D values; entries that cannot be converted are skipped.
     */
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

    /** @brief Get the sphere radius (world units, or pixels when fixedSize is true). */
    float size() const
    {
        return _size;
    }

    /**
     * @brief Set the sphere radius.
     * @param[in] size Radius, in world units or pixels depending on fixedSize.
     */
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

    /** @brief Returns true when spheres keep a constant screen size. */
    bool fixedSize() const
    {
        return _fixedSize;
    }

    /**
     * @brief Choose between constant screen size and world size.
     * @param[in] fixedSize True for a radius in pixels.
     */
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

    /** @brief Returns true when size or fixedSize changed since the renderable last consumed them. */
    bool paramsDirty() const
    {
        return _paramsDirty;
    }
    /** @brief Mark size and fixedSize as consumed by the renderable. */
    void clearParamsDirty()
    {
        _paramsDirty = false;
    }

    /** @brief Returns the sphere centers for the renderable. */
    const std::vector<QVector3D>& renderPositions() const
    {
        return _positions;
    }

    /** @brief Returns true when the positions changed since the renderable last consumed them. */
    bool positionsDirty() const
    {
        return _positionsDirty;
    }
    /** @brief Mark the positions as consumed by the renderable. */
    void clearPositionsDirty()
    {
        _positionsDirty = false;
    }

    /** @brief Bounds of the sphere centers; the sphere radius is not included. */
    BoundingBox boundingBox() const override
    {
        BoundingBox box;
        for (const QVector3D& position : _positions)
        {
            box.extend(position);
        }
        return box;
    }

    /** @brief Create a PointRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<PointRenderable>();
    }

  signals:
    /** @brief Emitted when the positions change. */
    void positionsChanged();
    /** @brief Emitted when the radius changes. */
    void sizeChanged();
    /** @brief Emitted when the sizing mode changes. */
    void fixedSizeChanged();

  private:
    /** @brief Sphere centers, in world coordinates. */
    std::vector<QVector3D> _positions;
    /** @brief Positions dirty flag. */
    bool _positionsDirty = false;
    /** @brief Sphere radius. */
    float _size = 20.0f;
    /** @brief Constant screen size flag. */
    bool _fixedSize = true;
    /** @brief Size parameters dirty flag. */
    bool _paramsDirty = true;
};
