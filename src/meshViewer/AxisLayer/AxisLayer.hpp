#pragma once

#include <Core/LayerItem.hpp>
#include <AxisLayer/AxisRenderable.hpp>

/** @brief Layer that renders a world-aligned XYZ axis gizmo at the camera orbit center (MotionInfo::getCenter()). */
class AxisLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

  public:
    /**
     * @brief Build the layer.
     * @param[in] parent Optional Qt parent object.
     */
    explicit AxisLayer(QObject* parent = nullptr)
      : LayerItem(parent)
    {}

    /** @brief Create an AxisRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override
    {
        return std::make_unique<AxisRenderable>();
    }

    /** @brief The gizmo is drawn on top of the scene. */
    bool rendersInForeground() const override
    {
        return true;
    }
};
