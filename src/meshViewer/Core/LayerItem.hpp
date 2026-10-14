#pragma once

#include <QObject>
#include <QString>
#include <QVector3D>
#include <qqml.h>
#include <memory>
#include <Core/BoundingBox.hpp>
#include <Core/Picking.hpp>

class IRenderable;

/**
 * @brief Abstract base for all declarative scene layers.
 *
 * Derive from this to create a new layer type. Registered as QML_UNCREATABLE
 * so that only concrete subclasses can be instantiated in QML.
 */
class LayerItem : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("LayerItem is an abstract base class")

    /** @brief Whether the layer is drawn. */
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    /** @brief Whether the layer takes part in picking queries. */
    Q_PROPERTY(bool picking READ picking WRITE setPicking NOTIFY pickingChanged)
    /** @brief Layer opacity, in [0, 1]. */
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)

  public:
    /**
     * @brief Build a visible, non-pickable, opaque layer.
     * @param[in] parent Optional Qt parent object.
     */
    explicit LayerItem(QObject* parent = nullptr);
    /** @brief Destroy the layer. */
    ~LayerItem() override = default;

    /** @brief Returns whether the layer is drawn. */
    bool visible() const
    {
        return _visible;
    }
    /**
     * @brief Show or hide the layer.
     * @param[in] v True to draw the layer.
     */
    void setVisible(bool v);

    /** @brief Returns whether the layer takes part in picking queries. */
    bool picking() const
    {
        return _picking;
    }
    /**
     * @brief Enable or disable picking on this layer.
     * @param[in] picking True to include the layer in picking queries.
     */
    void setPicking(bool picking);

    /**
     * @brief Returns the layer opacity factor.
     * @return Opacity in range [0.0, 1.0].
     */
    float opacity() const
    {
        return _opacity;
    }
    /**
     * @brief Sets the layer opacity factor.
     * @param opacity Requested opacity. Values are clamped to [0.0, 1.0].
     */
    void setOpacity(float opacity);

    /** @brief Factory method: create the IRenderable that renders this layer. */
    virtual std::unique_ptr<IRenderable> createRenderable() const = 0;

    /**
     * @brief Returns the bounding box of the layer content, in world coordinates.
     * @return The content bounds, or an empty box when the layer has no finite content (default).
     */
    Q_INVOKABLE virtual BoundingBox boundingBox() const
    {
        return {};
    }

    /** @brief Returns true if the layer must be drawn before all other layers (e.g. a background image). */
    virtual bool rendersInBackground() const
    {
        return false;
    }
    /** @brief Returns true if the layer must be drawn after all other layers (e.g. overlays). */
    virtual bool rendersInForeground() const
    {
        return false;
    }

    /** @brief Returns true if the layer implements pick(). */
    virtual bool canPick() const
    {
        return false;
    }
    /**
     * @brief Intersect a ray with the layer content.
     * @param[in] ray World-space ray.
     * @return The closest hit, or a result with @c hit set to false (default).
     */
    virtual LayerPickResult pick(const struct Ray& ray) const;
    /**
     * @brief Notify the layer that it holds the closest hit of a pick query.
     * @param[in] result The hit selected by SceneView.
     */
    virtual void applyPickResult(const LayerPickResult& result);
    /** @brief Notify the layer that the last pick query did not select it. */
    virtual void clearPick();

  signals:
    /** @brief Emitted when the visibility changes. */
    void visibleChanged();
    /** @brief Emitted when picking is enabled or disabled. */
    void pickingChanged();
    /** @brief Emitted when the opacity changes. */
    void opacityChanged();
    /** @brief Emitted when the layer content changed and must be re-synchronized and redrawn. */
    void dataReady();

  protected:
    /** @brief Visibility flag. */
    bool _visible = true;
    /** @brief Picking flag. */
    bool _picking = false;
    /** @brief Opacity, in [0, 1]. */
    float _opacity = 1.0f;
};
