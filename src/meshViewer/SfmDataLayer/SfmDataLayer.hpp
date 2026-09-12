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

    Q_PROPERTY(SfmDataObject* sfmData READ sfmData WRITE setSfmData NOTIFY sfmDataChanged)
    Q_PROPERTY(float pointSize READ pointSize WRITE setPointSize NOTIFY pointSizeChanged)
    Q_PROPERTY(float cameraSize READ cameraSize WRITE setCameraSize NOTIFY cameraSizeChanged)

  public:
    explicit SfmDataLayer(QObject* parent = nullptr);

    SfmDataObject* sfmData() const
    {
        return _sfmData;
    }
    void setSfmData(SfmDataObject* sfmData);

    float pointSize() const
    {
        return _pointSize;
    }
    void setPointSize(float pointSize);
    float cameraSize() const
    {
        return _cameraSize;
    }
    void setCameraSize(float cameraSize);

    // --- Render-thread accessors (call only from IRenderable::sync) ---
    float pointRadius() const
    {
        return _pointSize;
    }
    float renderedCameraScale() const
    {
        return _cameraSize;
    }
    bool dataDirty() const
    {
        return _dataDirty;
    }
    void clearDataDirty()
    {
        _dataDirty = false;
    }

    std::optional<size_t> selectedCameraIndex() const;

    /** @brief Cameras whose resectionId passes the sfmData's limitResectionId filter, in original order. */
    std::vector<SfmDataCameraInstance> visibleCameras() const;

    bool canPick() const override
    {
        return _sfmData && _sfmData->valid();
    }
    LayerPickResult pick(const Ray& ray) const override;
    void applyPickResult(const LayerPickResult& result) override;
    void clearPick() override;

  signals:
    void sfmDataChanged();
    void pointSizeChanged();
    void cameraSizeChanged();

  public:
    std::unique_ptr<IRenderable> createRenderable() const override;

  private:
    void onSfmDataReady();
    void onLimitResectionIdChanged();
    void rebuildPicker();

    SfmDataObject* _sfmData = nullptr;
    QMetaObject::Connection _sfmDataConnection;
    QMetaObject::Connection _sfmDataLimitConnection;
    bool _dataDirty = false;
    float _pointSize = 1.0f;
    float _cameraSize = 1.0f;
    SfmDataPicker _picker;
    mutable SfmDataPickHit _lastHit;
};