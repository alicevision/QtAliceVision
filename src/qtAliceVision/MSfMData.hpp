#pragma once

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>

#include <aliceVision/sfmData/SfMData.hpp>
#include <memory>

namespace qtAliceVision
{

/**
 * @brief QObject wrapper around a SfMData.
 * 
 * Given a path to a SfMData file,
 * the role of an MSfMData instance is to load the SfMData from disk.
 * This task is done asynchronously to avoid freezing the UI.
 * 
 * MSfMData objects are accesible from QML
 * and can be manipulated through their properties.
 * 
 * Note:
 * a SfMData contains important information for linking together various reconstruction data, such as:
 * - views (and their corresponding frame ID)
 * - poses (and their corresponding camera transform)
 * - intrinsics
 * - landmarks (and their corresponding observations, i.e. the 2D features used to build the landmark).
 */
class MSfMData : public QObject
{
    Q_OBJECT

    /// Data properties

    // Path to SfMData file
    Q_PROPERTY(QUrl sfmDataPath MEMBER _sfmDataPath NOTIFY sfmDataPathChanged)
    // Total number of reconstructed cameras
    Q_PROPERTY(size_t nbCameras READ nbCameras NOTIFY statusChanged)
    // View IDs of views in the SfMData
    Q_PROPERTY(QVariantList viewsIds READ getViewsIds NOTIFY viewsIdsChanged)

    /// Status

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    /// Status enum

    enum Status
    {
        None = 0,
        Loading,
        Ready,
        Error
    };
    Q_ENUM(Status)

    MSfMData();
    MSfMData& operator=(const MSfMData& other) = default;
    ~MSfMData() override;

private:
    MSfMData(const MSfMData& other);

public:
    /// Slots

    Q_SLOT void load();
    Q_SLOT void onSfmDataReady();
    Q_INVOKABLE QString getUrlFromViewId(int viewId);

    /// Signals

    Q_SIGNAL void sfmDataPathChanged();
    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void statusChanged(Status status);
    Q_SIGNAL void viewsIdsChanged();

public:
    const aliceVision::sfmData::SfMData& rawData() const { return *_sfmData; }
    aliceVision::sfmData::SfMData& rawData() { return *_sfmData; }
    const aliceVision::sfmData::SfMData* rawDataPtr() const { return _sfmData.get(); }

    Status status() const { return _status; }
    void setStatus(Status status);

    size_t nbCameras() const;

    QVariantList getViewsIds() const;

private:
    /// Private methods

    void clear();

    /// Private members

    QUrl _sfmDataPath;
    std::unique_ptr<aliceVision::sfmData::SfMData> _sfmData;
    std::unique_ptr<aliceVision::sfmData::SfMData> _loadingSfmData;
    Status _status = MSfMData::None;
    bool _outdated = false;
};

/**
 * @brief QRunnable object dedicated to loading a SfMData using AliceVision.
 */
class SfmDataIORunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit SfmDataIORunnable(const QUrl& sfmDataPath, aliceVision::sfmData::SfMData* sfmData)
        : _sfmDataPath(sfmDataPath)
        , _sfmData(sfmData)
    {
    }

    /// Load SfM based on input parameters
    Q_SLOT void run() override;

    /**
     * @brief  Emitted when sfmData have been loaded and sfmData objects created.
     */
    Q_SIGNAL void resultReady();

private:
    const QUrl _sfmDataPath;
    aliceVision::sfmData::SfMData* _sfmData;
};

} // namespace qtAliceVision
