#include <Core/LayerItem.hpp>

#include <algorithm>

LayerItem::LayerItem(QObject* parent)
  : QObject(parent)
{}

void LayerItem::setVisible(bool v)
{
    if (v == _visible)
    {
        return;
    }

    _visible = v;
    emit visibleChanged();
}

void LayerItem::setPicking(bool picking)
{
    if (picking == _picking)
    {
        return;
    }

    _picking = picking;
    emit pickingChanged();
}

void LayerItem::setOpacity(float opacity)
{
    const float clampedOpacity = std::clamp(opacity, 0.0f, 1.0f);
    if (qFuzzyCompare(_opacity, clampedOpacity))
    {
        return;
    }

    _opacity = clampedOpacity;
    emit opacityChanged();
    emit dataReady();
}

LayerPickResult LayerItem::pick(const Ray& ray) const
{
    Q_UNUSED(ray)
    return {};
}

void LayerItem::applyPickResult(const LayerPickResult& result)
{
    Q_UNUSED(result)
}

void LayerItem::clearPick()
{}
