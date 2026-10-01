#pragma once

#include <QVector3D>
#include <qqml.h>

#include <algorithm>

/**
 * @brief Axis-aligned 3D bounding box.
 *
 * A default-constructed box is empty (invalid): it contains no point and is neutral for union operations.
 * Accessors other than isValid() must only be used on a valid box.
 *
 * Exposed to QML as the `boundingBox` value type with read-only properties
 * (`valid`, `min`, `max`, `center`, `extent`, `maxExtent`) and the `united()` method.
 */
class BoundingBox
{
    Q_GADGET
    QML_VALUE_TYPE(boundingBox)

    Q_PROPERTY(bool valid READ isValid)
    Q_PROPERTY(QVector3D min READ min)
    Q_PROPERTY(QVector3D max READ max)
    Q_PROPERTY(QVector3D center READ center)
    Q_PROPERTY(QVector3D extent READ extent)
    Q_PROPERTY(float maxExtent READ maxExtent)

  public:
    BoundingBox() = default;

    /**
     * @brief Builds a valid box from two corners.
     * @param min Corner with the smallest coordinates.
     * @param max Corner with the largest coordinates.
     */
    BoundingBox(const QVector3D& min, const QVector3D& max)
      : _min(min),
        _max(max),
        _valid(true)
    {}

    /** @brief Returns true when the box contains at least one point. */
    bool isValid() const
    {
        return _valid;
    }

    /** @brief Returns the corner with the smallest coordinates. */
    const QVector3D& min() const
    {
        return _min;
    }

    /** @brief Returns the corner with the largest coordinates. */
    const QVector3D& max() const
    {
        return _max;
    }

    /** @brief Returns the center of the box. */
    QVector3D center() const
    {
        return (_min + _max) * 0.5f;
    }

    /** @brief Returns the size of the box along each axis. */
    QVector3D extent() const
    {
        return _max - _min;
    }

    /** @brief Returns the largest size of the box along any axis. */
    float maxExtent() const
    {
        const QVector3D e = extent();
        return std::max({e.x(), e.y(), e.z()});
    }

    /**
     * @brief Grows the box to contain a point.
     * @param point Point to include.
     */
    void extend(const QVector3D& point)
    {
        if (!_valid)
        {
            _min = point;
            _max = point;
            _valid = true;
            return;
        }

        _min = QVector3D(std::min(_min.x(), point.x()), std::min(_min.y(), point.y()), std::min(_min.z(), point.z()));
        _max = QVector3D(std::max(_max.x(), point.x()), std::max(_max.y(), point.y()), std::max(_max.z(), point.z()));
    }

    /**
     * @brief Grows the box to contain another box.
     * @param other Box to include. Ignored when empty.
     */
    void extend(const BoundingBox& other)
    {
        if (!other._valid)
        {
            return;
        }

        extend(other._min);
        extend(other._max);
    }

    /**
     * @brief Returns the smallest box containing both operands.
     * @param other Second operand.
     */
    Q_INVOKABLE BoundingBox united(const BoundingBox& other) const
    {
        BoundingBox result = *this;
        result.extend(other);
        return result;
    }

  private:
    QVector3D _min;
    QVector3D _max;
    bool _valid = false;
};
