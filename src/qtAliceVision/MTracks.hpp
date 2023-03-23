#pragma once

#include <aliceVision/track/Track.hpp>

#include <QQuickItem>
#include <QRunnable>
#include <QUrl>

#include <string>

namespace qtAliceVision
{

/**
 * @brief QObject wrapper around Tracks.
 * 
 * Given a folder containing feature matches,
 * the role of an MTracks instance is to load the matches from disk
 * and build the corresponding tracks.
 * These tasks are done asynchronously to avoid freezing the UI.
 * 
 * MTracks objects are accessible from QML
 * and can be manipulated through their properties.
 */
class MTracks : public QObject
{
    Q_OBJECT

    /// Data properties

    // Path to folder containing the matches
    Q_PROPERTY(QUrl matchingFolder MEMBER _matchingFolder NOTIFY matchingFolderChanged)

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

    /// Invokables

    Q_INVOKABLE int nbMatches(QString describerType, int viewId) const;

public:
    const aliceVision::track::TracksMap* tracksPtr() const { return _tracks; }
    const aliceVision::track::TracksMap& tracks() const { return *_tracks; }
    const aliceVision::track::TracksPerView& tracksPerView() const { return *_tracksPerView; }

    Status status() const { return _status; }
    void setStatus(Status status);

private:
    /// Private members

    QUrl _matchingFolder;

    aliceVision::track::TracksMap* _tracks = nullptr;
    aliceVision::track::TracksPerView* _tracksPerView = nullptr;

    bool _needReload = false;
    Status _status = MTracks::None;
};

/**
 * @brief QRunnable object dedicated to loading matches and building tracks using AliceVision.
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

    Q_SIGNAL void resultReady(aliceVision::track::TracksMap* tracks,
                              aliceVision::track::TracksPerView* tracksPerView);

private:
    const QUrl _matchingFolder;
};

} // namespace qtAliceVision
