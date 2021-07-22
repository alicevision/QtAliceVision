#pragma once

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>

#include <aliceVision/sfmData/SfMData.hpp>
#include <memory>

namespace qtAliceVision {

class SfmDataIORunnable;

/**
 * @brief QObject wrapper around a SfMData.
 */
class MSfMData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl sfmDataPath READ getSfmDataPath WRITE setSfmDataPath NOTIFY sfmDataPathChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(int nbCameras READ nbCameras NOTIFY statusChanged)

public:
    enum Status {
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
    Q_SLOT void load();
    Q_SLOT void onSfmDataReady();

public:
    Q_SIGNAL void sfmDataPathChanged();
    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void statusChanged(Status status);

private:
    void clear();

public:
    const aliceVision::sfmData::SfMData& rawData() const
    {
        return *_sfmData;
    }
    aliceVision::sfmData::SfMData& rawData()
    {
        return *_sfmData;
    }
    const aliceVision::sfmData::SfMData* rawDataPtr() const
    {
        return _sfmData.get();
    }

    QUrl getSfmDataPath() const { return _sfmDataPath; }
    void setSfmDataPath(const QUrl& sfmDataPath) {
        if(sfmDataPath == _sfmDataPath)
            return;
       _sfmDataPath = sfmDataPath;
       load();
       Q_EMIT sfmDataPathChanged();
    }

    Status status() const { return _status; }
    void setStatus(Status status) {
       if(status == _status)
           return;
       _status = status;
       Q_EMIT statusChanged(_status);
       if(status == Ready || status == Error)
       {
           Q_EMIT sfmDataChanged();
       }
    }

    inline int nbCameras() const {
        if(!_sfmData || _status != Ready)
            return 0;
        return _sfmData->getValidViews().size();
    }

private:
    QUrl _sfmDataPath;
    std::unique_ptr<aliceVision::sfmData::SfMData> _loadingSfmData;
    std::unique_ptr<aliceVision::sfmData::SfMData> _sfmData;
    Status _status = MSfMData::None;
};

}
