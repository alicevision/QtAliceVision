#pragma once

#include <Core/LayerItem.hpp>
#include <AxisLayer/AxisRenderable.hpp>

/** @brief Layer that renders a world-aligned XYZ axis gizmo at the current pick point. */
class AxisLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

  public:
    explicit AxisLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<AxisRenderable>();
    }

    bool rendersInForeground() const override
    {
        return true;
    }
};
