#pragma once

#include <QQuickRhiItem>
#include <QQmlListProperty>
#include <QMetaObject>
#include <QPointer>
#include <QVector2D>
#include <qqml.h>

#include <Core/Picking.hpp>
#include <Core/MotionInfo.hpp>
#include <Core/CameraInfo.hpp>

#include <Core/LayerItem.hpp>

class SceneRenderer;

class SceneView : public QQuickRhiItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQmlListProperty<LayerItem> layers READ layers NOTIFY layersChanged)
    Q_PROPERTY(LayerItem* pickingLayer READ pickingLayer NOTIFY pickingLayerChanged)
    Q_PROPERTY(int userCode READ userCode NOTIFY userCodeChanged)

    Q_PROPERTY(MotionInfo* motionInfo READ motionInfo WRITE setMotionInfo NOTIFY motionInfoChanged)
    Q_PROPERTY(CameraInfo* cameraInfo READ cameraInfo WRITE setCameraInfo NOTIFY cameraInfoChanged)

  public:
    explicit SceneView(QQuickItem* parent = nullptr);
    ~SceneView() override;

    QQuickRhiItemRenderer* createRenderer() override;

    QQmlListProperty<LayerItem> layers();
    const QList<LayerItem*>& layerList() const
    {
        return _layers;
    }

    LayerItem* pickingLayer() const
    {
        return _pickingLayer;
    }

    int userCode() const
    {
        return _userCode;
    }

    Q_INVOKABLE void appendLayer(LayerItem* layer);
    Q_INVOKABLE void removeLayer(LayerItem* layer);

    MotionInfo* motionInfo() const
    {
        return activeMotionInfo();
    }
    MotionInfo* getMotionInfo()
    {
        return activeMotionInfo();
    }
    const MotionInfo* getMotionInfo() const
    {
        return activeMotionInfo();
    }

    void setMotionInfo(MotionInfo* mi);

    CameraInfo* cameraInfo() const
    {
        return activeCameraInfo();
    }
    CameraInfo* getCameraInfo()
    {
        return activeCameraInfo();
    }
    const CameraInfo* getCameraInfo() const
    {
        return activeCameraInfo();
    }
    void setCameraInfo(CameraInfo* ci);

    Q_INVOKABLE void pick(const QVector2D& mousePos, int userCode);
    PendingPickRequest takePendingPickRequest();
    void applyLayerPick(LayerItem* layer, const LayerPickResult& result);

  signals:
    void layersChanged();
    void pickingLayerChanged();
    void userCodeChanged();
    void motionInfoChanged();
    void cameraInfoChanged();

  private:
    static void layersAppend(QQmlListProperty<LayerItem>* list, LayerItem* item);
    static qsizetype layersCount(QQmlListProperty<LayerItem>* list);
    static LayerItem* layersAt(QQmlListProperty<LayerItem>* list, qsizetype index);
    static void layersClear(QQmlListProperty<LayerItem>* list);

    /**
     * @brief Returns the active `MotionInfo`, or `nullptr` if none was set from QML.
     * @note Logs a `qWarning()` each time it is accessed while unset.
     */
    MotionInfo* activeMotionInfo() const;
    /**
     * @brief Returns the active `CameraInfo`, or `nullptr` if none was set from QML.
     * @note Logs a `qWarning()` each time it is accessed while unset.
     */
    CameraInfo* activeCameraInfo() const;
    void setPickingLayer(LayerItem* layer);

    void rebuildLayerConnections();

    QList<LayerItem*> _layers;
    LayerItem* _pickingLayer = nullptr;
    int _userCode = 0;
    QList<QMetaObject::Connection> _layerConnections;

    QPointer<MotionInfo> _motionInfo;
    QMetaObject::Connection _motionConnection;

    QPointer<CameraInfo> _cameraInfo;
    QMetaObject::Connection _cameraConnection;
    PendingPickRequest _pendingPickRequest;
};
