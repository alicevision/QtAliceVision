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

/**
 * @brief QML item rendering a stack of LayerItem objects with QRhi.
 *
 * The view is driven by a CameraInfo (projection) and a MotionInfo (view transform), both of
 * which must be set from QML. Rendering is delegated to a SceneRenderer on the render thread.
 *
 * Picking is asynchronous: pick() queues a request that is resolved during the next
 * synchronization; the result updates pickingLayer and userCode.
 */
class SceneView : public QQuickRhiItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief Layers drawn by the view, in draw order. */
    Q_PROPERTY(QQmlListProperty<LayerItem> layers READ layers NOTIFY layersChanged)
    /** @brief Layer hit by the last pick query, or null when nothing was hit. */
    Q_PROPERTY(LayerItem* pickingLayer READ pickingLayer NOTIFY pickingLayerChanged)
    /** @brief Union of the bounding boxes of all visible layers (empty when no visible layer has content). */
    Q_PROPERTY(BoundingBox boundingBox READ boundingBox NOTIFY boundingBoxChanged)
    /** @brief User code of the last successful pick query. */
    Q_PROPERTY(int userCode READ userCode NOTIFY userCodeChanged)

    /** @brief Camera motion providing the view transform. Must be set from QML. */
    Q_PROPERTY(MotionInfo* motionInfo READ motionInfo WRITE setMotionInfo NOTIFY motionInfoChanged)
    /** @brief Camera model providing the projection. Must be set from QML. */
    Q_PROPERTY(CameraInfo* cameraInfo READ cameraInfo WRITE setCameraInfo NOTIFY cameraInfoChanged)

  public:
    /**
     * @brief Build an empty view.
     * @param[in] parent Optional parent item.
     */
    explicit SceneView(QQuickItem* parent = nullptr);
    /** @brief Destroy the view. */
    ~SceneView() override;

    /**
     * @brief Create the SceneRenderer associated with this view.
     * @return A new renderer, owned by Qt Quick.
     */
    QQuickRhiItemRenderer* createRenderer() override;

    /** @brief Returns the QML list property exposing the layers. */
    QQmlListProperty<LayerItem> layers();
    /** @brief Returns the layers, in draw order. */
    const QList<LayerItem*>& layerList() const
    {
        return _layers;
    }

    /** @brief Returns the layer hit by the last pick query, or nullptr. */
    LayerItem* pickingLayer() const
    {
        return _pickingLayer;
    }

    /** @brief Returns the user code of the last successful pick query. */
    int userCode() const
    {
        return _userCode;
    }

    /**
     * @brief Returns the union of the bounding boxes of all visible layers.
     * @return The combined bounds, or an empty box when no visible layer has content.
     */
    BoundingBox boundingBox() const;

    /**
     * @brief Append a layer at the end of the draw order.
     * @param[in] layer Layer to add. Ignored when null or already present.
     */
    Q_INVOKABLE void appendLayer(LayerItem* layer);
    /**
     * @brief Remove a layer from the view.
     * @param[in] layer Layer to remove. Ignored when not present.
     */
    Q_INVOKABLE void removeLayer(LayerItem* layer);

    /** @brief Returns the active motion, or nullptr (with a warning) when unset. */
    MotionInfo* motionInfo() const
    {
        return activeMotionInfo();
    }
    /** @brief Returns the active motion, or nullptr (with a warning) when unset. */
    MotionInfo* getMotionInfo()
    {
        return activeMotionInfo();
    }
    /** @brief Returns the active motion, or nullptr (with a warning) when unset. */
    const MotionInfo* getMotionInfo() const
    {
        return activeMotionInfo();
    }

    /**
     * @brief Set the camera motion and redraw whenever it changes.
     * @param[in] mi Motion to use. Ignored when null or unchanged.
     */
    void setMotionInfo(MotionInfo* mi);

    /** @brief Returns the active camera, or nullptr (with a warning) when unset. */
    CameraInfo* cameraInfo() const
    {
        return activeCameraInfo();
    }
    /** @brief Returns the active camera, or nullptr (with a warning) when unset. */
    CameraInfo* getCameraInfo()
    {
        return activeCameraInfo();
    }
    /** @brief Returns the active camera, or nullptr (with a warning) when unset. */
    const CameraInfo* getCameraInfo() const
    {
        return activeCameraInfo();
    }
    /**
     * @brief Set the camera model and redraw whenever it changes.
     * @param[in] ci Camera to use. Ignored when null or unchanged.
     */
    void setCameraInfo(CameraInfo* ci);

    Q_INVOKABLE void pick(const QVector2D& mousePos, int userCode);
    /**
     * @brief Return the pending pick request and mark it as processed.
     * @return The request; its @c pending flag tells whether there was one.
     */
    PendingPickRequest takePendingPickRequest();
    /**
     * @brief Apply a pick result on the GUI thread.
     *
     * The hit layer receives LayerItem::applyPickResult(), all others LayerItem::clearPick().
     *
     * @param[in] layer Layer that was hit, or nullptr.
     * @param[in] result Pick result.
     */
    void applyLayerPick(LayerItem* layer, const LayerPickResult& result);

  signals:
    /** @brief Emitted when the layer list changes. */
    void layersChanged();
    /** @brief Emitted when the combined bounding box may have changed. */
    void boundingBoxChanged();
    /** @brief Emitted after each processed pick query. */
    void pickingLayerChanged();
    /** @brief Emitted when a successful pick query changes the user code. */
    void userCodeChanged();
    /** @brief Emitted when the motion object is replaced or modified. */
    void motionInfoChanged();
    /** @brief Emitted when the camera object is replaced or modified. */
    void cameraInfoChanged();

  private:
    /** @brief QQmlListProperty append callback. */
    static void layersAppend(QQmlListProperty<LayerItem>* list, LayerItem* item);
    /** @brief QQmlListProperty count callback. */
    static qsizetype layersCount(QQmlListProperty<LayerItem>* list);
    /** @brief QQmlListProperty at callback. */
    static LayerItem* layersAt(QQmlListProperty<LayerItem>* list, qsizetype index);
    /** @brief QQmlListProperty clear callback. */
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
    /**
     * @brief Set the picked layer and notify.
     * @param[in] layer Picked layer, or nullptr.
     */
    void setPickingLayer(LayerItem* layer);

    /** @brief Reconnect to the visibility and data signals of every layer. */
    void rebuildLayerConnections();

    /** @brief Layers, in draw order. */
    QList<LayerItem*> _layers;
    /** @brief Layer hit by the last pick query. */
    LayerItem* _pickingLayer = nullptr;
    /** @brief User code of the last successful pick query. */
    int _userCode = 0;
    /** @brief Connections to the layer signals. */
    QList<QMetaObject::Connection> _layerConnections;

    /** @brief Active camera motion. */
    QPointer<MotionInfo> _motionInfo;
    /** @brief Connection to MotionInfo::changed. */
    QMetaObject::Connection _motionConnection;

    /** @brief Active camera model. */
    QPointer<CameraInfo> _cameraInfo;
    /** @brief Connection to CameraInfo::changed. */
    QMetaObject::Connection _cameraConnection;
    /** @brief Pick request waiting for the next synchronization. */
    PendingPickRequest _pendingPickRequest;
};
