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

    /// Data properties

    // Path to folder containing the matches
    Q_PROPERTY(QUrl matchingFolder MEMBER _matchingFolder NOTIFY matchingFolderChanged)
    // Total number of tracks built from the matches
    Q_PROPERTY(size_t nbTracks READ nbTracks CONSTANT)

    /// Status

    Q_PROPERTY(Status status READ status NOTIFY statusChanged)

public:
    /// Status Enum

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
    /// Slots

    Q_SLOT void load();
    Q_SLOT void onReady(aliceVision::track::TracksMap* tracks, aliceVision::track::TracksPerView* tracksPerView);

    /// Signals

    Q_SIGNAL void matchingFolderChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void statusChanged(Status status);

public:
    const aliceVision::track::TracksMap* tracksPtr() const { return _tracks.get(); }
    const aliceVision::track::TracksMap& tracks() const { return *_tracks; }
    const aliceVision::track::TracksPerView& tracksPerView() const { return *_tracksPerView; }

    Status status() const { return _status; }
    void setStatus(Status status);

    size_t nbTracks() const;

private:
    /// Private methods

    void clear();

    /// Private members

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
