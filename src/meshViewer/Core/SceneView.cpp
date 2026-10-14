#include <Core/SceneView.hpp>
#include <Core/SceneRenderer.hpp>

SceneView::SceneView(QQuickItem* parent)
  : QQuickRhiItem(parent),
    _motionInfo(nullptr),
    _cameraInfo(nullptr)
{
    setFlag(QQuickItem::ItemHasContents, true);
}

SceneView::~SceneView() = default;

QQuickRhiItemRenderer* SceneView::createRenderer()
{
    return new SceneRenderer();
}

QQmlListProperty<LayerItem> SceneView::layers()
{
    return QQmlListProperty<LayerItem>(
      this, nullptr, &SceneView::layersAppend, &SceneView::layersCount, &SceneView::layersAt, &SceneView::layersClear);
}

BoundingBox SceneView::boundingBox() const
{
    BoundingBox box;
    for (const LayerItem* layer : _layers)
    {
        if (layer && layer->visible())
        {
            box.extend(layer->boundingBox());
        }
    }
    return box;
}

void SceneView::layersAppend(QQmlListProperty<LayerItem>* list, LayerItem* item)
{
    auto* self = static_cast<SceneView*>(list->object);
    self->_layers.append(item);
    self->rebuildLayerConnections();
    emit self->layersChanged();
    self->update();
}

qsizetype SceneView::layersCount(QQmlListProperty<LayerItem>* list)
{
    return static_cast<SceneView*>(list->object)->_layers.count();
}

LayerItem* SceneView::layersAt(QQmlListProperty<LayerItem>* list, qsizetype index)
{
    return static_cast<SceneView*>(list->object)->_layers.at(index);
}

void SceneView::layersClear(QQmlListProperty<LayerItem>* list)
{
    auto* self = static_cast<SceneView*>(list->object);
    self->_layers.clear();
    self->rebuildLayerConnections();
    emit self->layersChanged();
    self->update();
}

void SceneView::appendLayer(LayerItem* layer)
{
    if (!layer || _layers.contains(layer))
    {
        return;
    }

    _layers.append(layer);
    rebuildLayerConnections();
    emit layersChanged();
    update();
}

void SceneView::removeLayer(LayerItem* layer)
{
    if (_layers.removeOne(layer))
    {
        rebuildLayerConnections();
        emit layersChanged();
        update();
    }
}

void SceneView::rebuildLayerConnections()
{
    for (auto& conn : _layerConnections)
    {
        QObject::disconnect(conn);
    }

    _layerConnections.clear();

    for (LayerItem* layer : _layers)
    {
        _layerConnections += connect(layer, &LayerItem::visibleChanged, this, [this]() {
            emit boundingBoxChanged();
            update();
        });
        _layerConnections += connect(layer, &LayerItem::dataReady, this, [this]() {
            emit boundingBoxChanged();
            update();
            polish();
        });
    }

    emit boundingBoxChanged();
}

// ── Camera / motion ──────────────────────────────────────────────────────────

MotionInfo* SceneView::activeMotionInfo() const
{
    if (!_motionInfo)
    {
        qWarning() << "SceneView: motionInfo property was not set from QML.";
        return nullptr;
    }

    return _motionInfo.data();
}

CameraInfo* SceneView::activeCameraInfo() const
{
    if (!_cameraInfo)
    {
        qWarning() << "SceneView: cameraInfo property was not set from QML.";
        return nullptr;
    }

    return _cameraInfo.data();
}

void SceneView::setMotionInfo(MotionInfo* mi)
{
    if (mi == nullptr || mi == _motionInfo)
        return;

    if (_motionConnection)
        QObject::disconnect(_motionConnection);

    _motionInfo = mi;

    _motionConnection = connect(_motionInfo, &MotionInfo::changed, this, [this]() {
        emit motionInfoChanged();
        update();
    });

    connect(_motionInfo, &QObject::destroyed, this, [this]() {
        emit motionInfoChanged();
        update();
    });

    emit motionInfoChanged();
    update();
}

void SceneView::setCameraInfo(CameraInfo* ci)
{
    if (ci == nullptr || ci == _cameraInfo)
        return;

    if (_cameraConnection)
        QObject::disconnect(_cameraConnection);

    _cameraInfo = ci;

    _cameraConnection = connect(_cameraInfo, &CameraInfo::changed, this, [this]() {
        emit cameraInfoChanged();
        update();
    });

    connect(_cameraInfo, &QObject::destroyed, this, [this]() {
        emit cameraInfoChanged();
        update();
    });

    emit cameraInfoChanged();
    update();
}

QMatrix4x4 SceneView::viewMatrix() const
{
    return _motionInfo ? _motionInfo->getMatrix() : QMatrix4x4{};
}

QMatrix4x4 SceneView::projectionMatrix() const
{
    if (!_cameraInfo)
    {
        return {};
    }

    const double aspectRatio = height() > 0 ? width() / height() : 1.0;
    return _cameraInfo->getProjectionMatrix(aspectRatio);
}

QVector2D SceneView::worldToScreen(const QVector3D& point) const
{
    const QVector4D clip = projectionMatrix() * viewMatrix() * QVector4D(point, 1.0f);
    if (qFuzzyIsNull(clip.w()))
    {
        return {};
    }

    // NDC → screen, inverse of the mapping in unprojectRay()
    const float ndcX = clip.x() / clip.w();
    const float ndcY = clip.y() / clip.w();
    return QVector2D((ndcX + 1.0f) * 0.5f * float(width()), (1.0f - ndcY) * 0.5f * float(height()));
}

QVector3D SceneView::cameraPosition() const
{
    return viewMatrix().inverted().column(3).toVector3D();
}

Ray SceneView::screenRay(const QVector2D& mousePos) const
{
    return unprojectRay(projectionMatrix(), viewMatrix(), mousePos, float(width()), float(height()));
}

void SceneView::pick(const QVector2D& mousePos, int userCode)
{
    _pendingPickRequest.pending = true;
    _pendingPickRequest.userCode = userCode;
    _pendingPickRequest.mousePos = mousePos;
    update();
}

PendingPickRequest SceneView::takePendingPickRequest()
{
    PendingPickRequest request = _pendingPickRequest;
    _pendingPickRequest.pending = false;
    return request;
}

void SceneView::applyLayerPick(LayerItem* layer, const LayerPickResult& result)
{
    for (LayerItem* candidate : _layers)
    {
        if (!candidate)
        {
            continue;
        }

        if (candidate == layer && result.hit)
        {
            candidate->applyPickResult(result);
        }
        else
        {
            candidate->clearPick();
        }
    }

    if (result.hit && _userCode != result.userCode)
    {
        _userCode = result.userCode;
        emit userCodeChanged();
    }

    setPickingLayer(result.hit ? layer : nullptr);

    update();
    polish();
}

void SceneView::setPickingLayer(LayerItem* layer)
{
    _pickingLayer = layer;
    emit pickingLayerChanged();
}
