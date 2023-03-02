#pragma once

#include <aliceVision/track/Track.hpp>

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>

namespace qtAliceVision
{

class TracksIORunnable;

/**
 * @brief QObject wrapper around Tracks.
 */
class MTracks : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl matchingFolder READ getMatchingFolder WRITE setMatchingFolder NOTIFY matchingFolderChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(size_t nbTracks READ nbTracks CONSTANT)

public:
    enum Status
    {
        None = 0,
        Loading,
        Ready,
        Error
    };
    Q_ENUM(Status)

    MTracks();
    MTracks& operator=(const MTracks& other) = default;
    ~MTracks() override;

private:
    MTracks(const MTracks& other);

public:
    Q_SLOT void load();
    Q_SLOT void onReady(aliceVision::track::TracksMap* tracks, aliceVision::track::TracksPerView* tracksPerView);

public:
    Q_SIGNAL void matchingFolderChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void statusChanged(Status status);

private:
    void clear()
    {
        if (_tracks)
            _tracks->clear();
        if (_tracksPerView)
            _tracksPerView->clear();
        qInfo() << "[QtAliceVision] MTracks clear";
        Q_EMIT tracksChanged();
    }

public:
    const aliceVision::track::TracksMap* tracksPtr() const { return _tracks.get(); }
    const aliceVision::track::TracksMap& tracks() const { return *_tracks; }
    const aliceVision::track::TracksPerView& tracksPerView() const { return *_tracksPerView; }

    QUrl getMatchingFolder() const { return _matchingFolder; }
    void setMatchingFolder(const QUrl& matchingFolder)
    {
        if (matchingFolder == _matchingFolder)
            return;
        _matchingFolder = matchingFolder;
        Q_EMIT matchingFolderChanged();
    }

    Status status() const { return _status; }
    void setStatus(Status status)
    {
        if (status == _status)
            return;
        _status = status;
        Q_EMIT statusChanged(_status);
        if (status == Ready || status == Error)
        {
            Q_EMIT tracksChanged();
        }
    }

    inline size_t nbTracks() const
    {
        if (!_tracks || _status != MTracks::Ready)
            return 0;
        return _tracks->size();
    }

private:
    QUrl _matchingFolder;
    std::unique_ptr<aliceVision::track::TracksMap> _tracks;
    std::unique_ptr<aliceVision::track::TracksPerView> _tracksPerView;
    Status _status = MTracks::None;
};

/**
 * @brief QRunnable object dedicated to load sfmData using AliceVision.
 */
class TracksIORunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit TracksIORunnable(const QUrl& matchingFolder)
        : _matchingFolder(matchingFolder)
    {
    }

    Q_SLOT void run() override;

    Q_SIGNAL void resultReady(aliceVision::track::TracksMap* _tracks,
                              aliceVision::track::TracksPerView* _tracksPerView);

private:
    const QUrl _matchingFolder;
};

} // namespace qtAliceVision
