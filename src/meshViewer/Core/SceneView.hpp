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

    /**
     * @brief Returns the user code associated with the last successful pick.
     * @return The `userCode` value passed to pick(), or 0 if no pick has hit a layer yet.
     */
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

    MotionInfo* activeMotionInfo() const;
    CameraInfo* activeCameraInfo() const;
    void setPickingLayer(LayerItem* layer);

    void rebuildLayerConnections();

    QList<LayerItem*> _layers;
    LayerItem* _pickingLayer = nullptr;
    int _userCode = 0;
    QList<QMetaObject::Connection> _layerConnections;

    MotionInfo _defaultMotionInfo;
    QPointer<MotionInfo> _motionInfo;
    QMetaObject::Connection _motionConnection;
    QMetaObject::Connection _defaultMotionConnection;

    CameraInfo _defaultCameraInfo;
    QPointer<CameraInfo> _cameraInfo;
    QMetaObject::Connection _cameraConnection;
    QMetaObject::Connection _defaultCameraConnection;
    PendingPickRequest _pendingPickRequest;
};
