#pragma once

#include <Core/LayerItem.hpp>
#include <SfmDataLayer/SfmDataObject.hpp>
#include <SfmDataLayer/SfmDataPicker.hpp>

#include <QMetaObject>
#include <optional>

/**
 * @brief Layer that renders a SfmDataObject.
 *
 * Owns display parameters (pointSize, cameraSize) and the picking BVH.
 * The underlying geometry is provided by a shared SfmDataObject.
 */
class SfmDataLayer : public LayerItem
{
    Q_OBJECT
    QML_ELEMENT

    /** @brief SfM data to draw. The layer does not take ownership. */
    Q_PROPERTY(SfmDataObject* sfmData READ sfmData WRITE setSfmData NOTIFY sfmDataChanged)
    /** @brief Landmark size, in pixels. */
    Q_PROPERTY(float pointSize READ pointSize WRITE setPointSize NOTIFY pointSizeChanged)
    /** @brief Scale of the camera shapes. */
    Q_PROPERTY(float cameraSize READ cameraSize WRITE setCameraSize NOTIFY cameraSizeChanged)
    /** @brief View id of the highlighted camera, or UndefinedIndexT. */
    Q_PROPERTY(quint32 selectedCamera READ selectedCamera WRITE setSelectedCamera NOTIFY selectedCameraChanged)

  public:
    /**
     * @brief Build a layer without data.
     * @param[in] parent Optional Qt parent object.
     */
    explicit SfmDataLayer(QObject* parent = nullptr);

    /** @brief Get the drawn SfM data. */
    SfmDataObject* sfmData() const
    {
        return _sfmData;
    }
    /**
     * @brief Set the SfM data to draw and follow its updates.
     * @param[in] sfmData SfM data object, or nullptr.
     */
    void setSfmData(SfmDataObject* sfmData);

    /** @brief Get the landmark size, in pixels. */
    float pointSize() const
    {
        return _pointSize;
    }
    /**
     * @brief Set the landmark size.
     * @param[in] pointSize Size, in pixels.
     */
    void setPointSize(float pointSize);
    /** @brief Get the camera shape scale. */
    float cameraSize() const
    {
        return _cameraSize;
    }
    /**
     * @brief Set the camera shape scale.
     * @param[in] cameraSize Scale.
     */
    void setCameraSize(float cameraSize);

    /** @brief Get the view id of the highlighted camera. */
    quint32 selectedCamera() const
    {
        return _selectedCamera;
    }
    /**
     * @brief Highlight a camera.
     * @param[in] selectedCamera View id, or UndefinedIndexT.
     */
    void setSelectedCamera(quint32 selectedCamera);

    /** @name Render-thread accessors (call only from IRenderable::sync())
     *  @{ */
    /** @brief Returns the landmark size for the renderable. */
    float pointRadius() const
    {
        return _pointSize;
    }
    /** @brief Returns the camera scale for the renderable. */
    float renderedCameraScale() const
    {
        return _cameraSize;
    }
    /** @brief Returns true when the data changed since the renderable last consumed it. */
    bool dataDirty() const
    {
        return _dataDirty;
    }
    /** @brief Mark the data as consumed by the renderable. */
    void clearDataDirty()
    {
        _dataDirty = false;
    }
    /** @} */

    /** @brief Cameras whose resectionId passes the sfmData's limitResectionId filter, in original order. */
    std::vector<SfmDataCameraInstance> visibleCameras() const;

    /** @brief Union of the point cloud and camera center bounds. */
    BoundingBox boundingBox() const override
    {
        if (!_sfmData || !_sfmData->valid())
        {
            return {};
        }
        const SfmDataContent& content = _sfmData->content();
        return content.pointCloudBoundingBox.united(content.cameraCenterBoundingBox);
    }

    /** @brief Returns true when valid SfM data is set. */
    bool canPick() const override
    {
        return _sfmData && _sfmData->valid();
    }
    /**
     * @brief Intersect the ray with the landmarks and visible cameras.
     * @param[in] ray World-space ray.
     * @return The closest hit; the hit details are kept for applyPickResult().
     */
    LayerPickResult pick(const Ray& ray) const override;
    /**
     * @brief Select the element of the last hit as the highlighted camera.
     * @param[in] result Pick result.
     */
    void applyPickResult(const LayerPickResult& result) override;
    /** @brief Forget the last hit. */
    void clearPick() override;

  signals:
    /** @brief Emitted when the SfM data object is replaced. */
    void sfmDataChanged();
    /** @brief Emitted when the landmark size changes. */
    void pointSizeChanged();
    /** @brief Emitted when the camera scale changes. */
    void cameraSizeChanged();
    /** @brief Emitted when the highlighted camera changes. */
    void selectedCameraChanged();

  public:
    /** @brief Create an SfmDataRenderable. */
    std::unique_ptr<IRenderable> createRenderable() const override;

  private:
    /** @brief Flag the data dirty and rebuild the picker after the SfM data changed. */
    void onSfmDataReady();
    /** @brief Flag the data dirty and rebuild the picker when the camera filter changed. */
    void onLimitResectionIdChanged();
    /** @brief Rebuild the picking BVH from the landmarks and visible cameras. */
    void rebuildPicker();

    /** @brief Drawn SfM data, not owned. */
    SfmDataObject* _sfmData = nullptr;
    /** @brief Connection to SfmDataObject::dataReady. */
    QMetaObject::Connection _sfmDataConnection;
    /** @brief Connection to SfmDataObject::limitResectionIdChanged. */
    QMetaObject::Connection _sfmDataLimitConnection;
    /** @brief Data dirty flag. */
    bool _dataDirty = false;
    /** @brief Landmark size, in pixels. */
    float _pointSize = 1.0f;
    /** @brief Camera shape scale. */
    float _cameraSize = 1.0f;
    /** @brief View id of the highlighted camera. */
    quint32 _selectedCamera = aliceVision::UndefinedIndexT;
    /** @brief Ray-casting acceleration structure. */
    SfmDataPicker _picker;
    /** @brief Details of the last hit, set by pick(). */
    mutable SfmDataPickHit _lastHit;
};