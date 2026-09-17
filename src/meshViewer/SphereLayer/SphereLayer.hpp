#pragma once

#include <Core/LayerItem.hpp>
#include <SphereLayer/SphereRenderable.hpp>

#include <QVariantList>

/** @brief Layer that renders GPU-instanced spheres at each picked point. */
class SphereLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList positions READ positions WRITE setPositions NOTIFY positionsChanged)

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

  private:
    std::vector<QVector3D> _positions;
    bool _positionsDirty = false;
};
