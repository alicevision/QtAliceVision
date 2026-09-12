#include <Core/SceneView.hpp>
#include <Core/SceneRenderer.hpp>

SceneView::SceneView(QQuickItem* parent)
  : QQuickRhiItem(parent),
    _motionInfo(nullptr),
    _cameraInfo(nullptr)
{
    setFlag(QQuickItem::ItemHasContents, true);

    _defaultMotionConnection = connect(&_defaultMotionInfo, &MotionInfo::changed, this, [this]() {
        emit motionInfoChanged();
        update();
    });
    _defaultCameraConnection = connect(&_defaultCameraInfo, &CameraInfo::changed, this, [this]() {
        emit cameraInfoChanged();
        update();
    });
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
        _layerConnections += connect(layer, &LayerItem::visibleChanged, this, [this]() { update(); });
        _layerConnections += connect(layer, &LayerItem::dataReady, this, [this]() {
            update();
            polish();
        });
    }
}

// ── Camera / motion ──────────────────────────────────────────────────────────

MotionInfo* SceneView::activeMotionInfo() const
{
    return _motionInfo ? _motionInfo.data() : const_cast<MotionInfo*>(&_defaultMotionInfo);
}

CameraInfo* SceneView::activeCameraInfo() const
{
    return _cameraInfo ? _cameraInfo.data() : const_cast<CameraInfo*>(&_defaultCameraInfo);
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
