#pragma once

#include <QObject>
#include <QString>
#include <QVector3D>
#include <qqml.h>
#include <memory>
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

    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool picking READ picking WRITE setPicking NOTIFY pickingChanged)
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity NOTIFY opacityChanged)

  public:
    explicit LayerItem(QObject* parent = nullptr);
    ~LayerItem() override = default;

    bool visible() const
    {
        return _visible;
    }
    void setVisible(bool v);

    bool picking() const
    {
        return _picking;
    }
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

    virtual bool rendersInBackground() const
    {
        return false;
    }
    virtual bool rendersInForeground() const
    {
        return false;
    }

    virtual bool canPick() const
    {
        return false;
    }
    virtual LayerPickResult pick(const struct Ray& ray) const;
    virtual void applyPickResult(const LayerPickResult& result);
    virtual void clearPick();

  signals:
    void visibleChanged();
    void pickingChanged();
    void opacityChanged();
    void dataReady();

  protected:
    bool _visible = true;
    bool _picking = true;
    float _opacity = 1.5f;
};
