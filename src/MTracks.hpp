#pragma once

#include <aliceVision/track/Track.hpp>

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>


namespace qtAliceVision {

class TracksIORunnable;

/**
 * @brief QObject wrapper around Tracks.
 */
class MTracks : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl matchingFolder READ getMatchingFolder WRITE setMatchingFolder NOTIFY matchingFolderChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(int nbTracks READ nbTracks CONSTANT)

public:
    enum Status {
            None = 0,
            Loading,
            Ready,
            Error
    };
    Q_ENUM(Status)

    MTracks() = default;
    MTracks& operator=(const MTracks& other) = default;
    ~MTracks() = default;

private:
    MTracks(const MTracks& other);

public:
    Q_SLOT void load();
    Q_SLOT void onReady();

public:
    Q_SIGNAL void matchingFolderChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void statusChanged(Status status);

private:
    void clear()
    {
        _tracks.clear();
        _tracksPerView.clear();
        qWarning() << "[QtAliceVision] MTracks clear";
        Q_EMIT tracksChanged();
    }

public:
    const aliceVision::track::TracksMap& tracks() const
    {
        return _tracks;
    }
    const aliceVision::track::TracksPerView& tracksPerView() const
    {
        return _tracksPerView;
    }

    QUrl getMatchingFolder() const { return _matchingFolder; }
    void setMatchingFolder(const QUrl& matchingFolder) {
       _matchingFolder = matchingFolder;
       load();
       Q_EMIT matchingFolderChanged();
    }

    Status status() const { return _status; }
    void setStatus(Status status) {
       if(status == _status)
           return;
       _status = status;
       Q_EMIT statusChanged(_status);
       if(status == Ready)
       {
           Q_EMIT tracksChanged();
       }
   }

    inline int nbTracks() const {
        if(_status != MTracks::Ready)
            return 0;
        return _tracks.size();
    }

private:
    QUrl _matchingFolder;
    aliceVision::track::TracksMap _tracks;
    aliceVision::track::TracksPerView _tracksPerView;
    Status _status = MTracks::None;

    TracksIORunnable* _ioRunnable = nullptr;
};

}
