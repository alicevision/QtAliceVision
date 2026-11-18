#pragma once

#include <Core/LayerItem.hpp>
#include <PointLayer/PointRenderable.hpp>

#include <QColor>
#include <QVariantList>

/** @brief Layer that renders a GPU-instanced Maya-style locator (3D cross) at each point. */
class PointLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Picked point positions (QVector3D values) where locators are drawn. */
    Q_PROPERTY(QVariantList positions READ positions WRITE setPositions NOTIFY positionsChanged)

    /**
     * @brief Locator half-axis length.
     * Expressed in world units, or in pixels when @ref fixedSize is true.
     */
    Q_PROPERTY(float size READ size WRITE setSize NOTIFY sizeChanged)

    /** @brief If true, locators keep a constant screen size whatever their distance to the camera. */
    Q_PROPERTY(bool fixedSize READ fixedSize WRITE setFixedSize NOTIFY fixedSizeChanged)

    /** @brief Index in @ref positions of the selected point, or -1 when no point is selected. */
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)

    /** @brief Color of the unselected locators. */
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

    /** @brief Color of the locator at @ref selectedIndex. */
    Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor NOTIFY selectedColorChanged)

  public:
    /**
     * @brief Build an empty layer.
     * @param[in] parent Optional Qt parent object.
     */
    explicit PointLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    /** @brief Get the locator centers as a list of QVector3D values. */
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
     * @brief Replace the locator centers.
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
        _instancesDirty = true;
        emit positionsChanged();
        emit dataReady();
    }

    /** @brief Get the locator half-axis length (world units, or pixels when fixedSize is true). */
    float size() const
    {
        return _size;
    }

    /**
     * @brief Set the locator half-axis length.
     * @param[in] size Half-axis length, in world units or pixels depending on fixedSize.
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

    /** @brief Returns true when locators keep a constant screen size. */
    bool fixedSize() const
    {
        return _fixedSize;
    }

    /**
     * @brief Choose between constant screen size and world size.
     * @param[in] fixedSize True for a size in pixels.
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

    /** @brief Get the index of the selected point, or -1 when none is selected. */
    int selectedIndex() const
    {
        return _selectedIndex;
    }

    /**
     * @brief Select a point.
     * @param[in] selectedIndex Index in positions; -1 (or any out-of-range value) selects nothing.
     */
    void setSelectedIndex(int selectedIndex)
    {
        if (_selectedIndex == selectedIndex)
        {
            return;
        }

        _selectedIndex = selectedIndex;
        _instancesDirty = true;
        emit selectedIndexChanged();
        emit dataReady();
    }

    /** @brief Get the color of the unselected locators. */
    QColor color() const
    {
        return _color;
    }

    /**
     * @brief Set the color of the unselected locators.
     * @param[in] color New color; alpha is ignored.
     */
    void setColor(const QColor& color)
    {
        if (_color == color)
        {
            return;
        }

        _color = color;
        _instancesDirty = true;
        emit colorChanged();
        emit dataReady();
    }

    /** @brief Get the color of the selected locator. */
    QColor selectedColor() const
    {
        return _selectedColor;
    }

    /**
     * @brief Set the color of the selected locator.
     * @param[in] selectedColor New color; alpha is ignored.
     */
    void setSelectedColor(const QColor& selectedColor)
    {
        if (_selectedColor == selectedColor)
        {
            return;
        }

        _selectedColor = selectedColor;
        _instancesDirty = true;
        emit selectedColorChanged();
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

    /** @brief Returns the locator centers for the renderable. */
    const std::vector<QVector3D>& renderPositions() const
    {
        return _positions;
    }

    /** @brief Returns true when the positions, selection or colors changed since the renderable last consumed them. */
    bool instancesDirty() const
    {
        return _instancesDirty;
    }
    /** @brief Mark the positions, selection and colors as consumed by the renderable. */
    void clearInstancesDirty()
    {
        _instancesDirty = false;
    }

    /** @brief Bounds of the locator centers; the locator size is not included. */
    BoundingBox boundingBox() const override
    {
        BoundingBox box;
        for (const QVector3D& position : _positions)
        {
            box.extend(position);
        }
        return box;
    }

    /** @brief Mark all layer data dirty so a recreated renderable receives it again. */
    void markAllDirty() override
    {
        _instancesDirty = true;
        _paramsDirty = true;
    }

    /** @brief Create a PointRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<PointRenderable>();
    }

  signals:
    /** @brief Emitted when the positions change. */
    void positionsChanged();
    /** @brief Emitted when the size changes. */
    void sizeChanged();
    /** @brief Emitted when the sizing mode changes. */
    void fixedSizeChanged();
    /** @brief Emitted when the selected point changes. */
    void selectedIndexChanged();
    /** @brief Emitted when the unselected locator color changes. */
    void colorChanged();
    /** @brief Emitted when the selected locator color changes. */
    void selectedColorChanged();

  private:
    /** @brief Locator centers, in world coordinates. */
    std::vector<QVector3D> _positions;
    /** @brief Index of the selected point, -1 for none. */
    int _selectedIndex = -1;
    /** @brief Color of the unselected locators. */
    QColor _color = QColor::fromRgbF(1.0f, 1.0f, 0.0f);
    /** @brief Color of the selected locator. */
    QColor _selectedColor = QColor::fromRgbF(1.0f, 0.3f, 0.0f);
    /** @brief Positions, selection and colors dirty flag. */
    bool _instancesDirty = false;
    /** @brief Locator half-axis length. */
    float _size = 20.0f;
    /** @brief Constant screen size flag. */
    bool _fixedSize = true;
    /** @brief Size parameters dirty flag. */
    bool _paramsDirty = true;
};
