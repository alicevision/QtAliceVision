#pragma once

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>

#include <aliceVision/sfmData/SfMData.hpp>

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
    Q_PROPERTY(int nbCameras READ nbCameras CONSTANT)

public:
    enum Status {
            None = 0,
            Loading,
            Ready,
            Error
    };
    Q_ENUM(Status)

    MSfMData() = default;
    MSfMData& operator=(const MSfMData& other) = default;
    ~MSfMData() = default;

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
    void clear()
    {
        _sfmData.reset(new aliceVision::sfmData::SfMData());
        qWarning() << "[QtAliceVision] MSfMData.hpp clear : ";
        Q_EMIT sfmDataChanged();
    }

public:
    const aliceVision::sfmData::SfMData& rawData() const
    {
        return *_sfmData;
    }
    aliceVision::sfmData::SfMData& rawData()
    {
        return *_sfmData;
    }

    QUrl getSfmDataPath() const { return _sfmDataPath; }
    void setSfmDataPath(const QUrl& sfmDataPath) {
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
           if(status == Ready)
           {
               Q_EMIT sfmDataChanged();
           }
       }

    inline int nbCameras() const {
        if(!_sfmData)
            return 0;
        return _sfmData->getValidViews().size();
    }

private:
    QUrl _sfmDataPath;
    std::unique_ptr<aliceVision::sfmData::SfMData> _sfmData;
    Status _status = MSfMData::None;

    SfmDataIORunnable* _ioRunnable = nullptr;
};

}
