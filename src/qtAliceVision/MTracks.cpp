#include "MTracks.hpp"

#include <aliceVision/matching/io.hpp>
#include <aliceVision/track/TracksBuilder.hpp>
#include <aliceVision/track/tracksUtils.hpp>

#include <QDebug>
#include <QFileInfo>
#include <QThreadPool>

namespace qtAliceVision
{

void TracksIORunnable::run()
{
    aliceVision::track::TracksMap* tracks = new aliceVision::track::TracksMap;
    aliceVision::track::TracksPerView* tracksPerView = new aliceVision::track::TracksPerView;
    try
    {
        aliceVision::matching::PairwiseMatches pairwiseMatches;
        if (!aliceVision::matching::Load(pairwiseMatches,
                                         /*viewsKeysFilter=*/{}, {_matchingFolder.toLocalFile().toStdString()},
                                         /*descTypes=*/{},
                                         /*maxNbMatches=*/0,
                                         /*minNbMatches=*/0))
        {
            qDebug() << "[QtAliceVision] Failed to load matches: " << _matchingFolder << ".";
        }
        aliceVision::track::TracksBuilder tracksBuilder;
        tracksBuilder.build(pairwiseMatches);
        tracksBuilder.exportToSTL(*tracks);
        aliceVision::track::computeTracksPerView(*tracks, *tracksPerView);
    }
    catch (std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load matches: " << _matchingFolder << "."
                 << "\n"
                 << e.what();
    }

    Q_EMIT resultReady(tracks, tracksPerView);
}

MTracks::MTracks()
{
    connect(this, &MTracks::matchingFolderChanged, this, &MTracks::load);
}

MTracks::~MTracks()
{
    if (_tracks) delete _tracks;
    if (_tracksPerView) delete _tracksPerView;

    setStatus(None);
}

void MTracks::load()
{
    _needReload = false;

    if (_status == Loading)
    {
        qDebug("[QtAliceVision] Tracks: Unable to load, a load event is already running.");
        _needReload = true;
        return;
    }

    qDebug() << "MTracks::load _matchingFolder: " << _matchingFolder;
    if (_matchingFolder.isEmpty())
    {
        setStatus(None);
        return;
    }
    if (!QFileInfo::exists(_matchingFolder.toLocalFile()))
    {
        setStatus(Error);
        return;
    }

    setStatus(Loading);
    qDebug() << "MTracks::load Start loading _matchingFolder: " << _matchingFolder;

    // load matches from file in a seperate thread
    TracksIORunnable* ioRunnable = new TracksIORunnable(_matchingFolder);
    connect(ioRunnable, &TracksIORunnable::resultReady, this, &MTracks::onReady);
    QThreadPool::globalInstance()->start(ioRunnable);
}

void MTracks::onReady(aliceVision::track::TracksMap* tracks, aliceVision::track::TracksPerView* tracksPerView)
{
    if (_needReload)
    {
        if (tracks) delete tracks;
        if (tracksPerView) delete tracksPerView;

        setStatus(None);
        load();
        return;
    }

    if (tracks && tracksPerView)
    {
        if (_tracks) delete _tracks;
        if (_tracksPerView) delete _tracksPerView;

        _tracks = tracks;
        _tracksPerView = tracksPerView;
    }
    else if (tracks)
    {
        delete tracks;
    }
    else if (tracksPerView)
    {
        delete tracksPerView;
    }
    
    setStatus(Ready);
}

void MTracks::setStatus(Status status)
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

int MTracks::nbMatches(QString describerType, int viewId) const
{
    if (_status != Ready)
    {
        return 0;
    }

    if (!_tracksPerView)
    {
        return 0;
    }

    const auto trackIdsIt = _tracksPerView->find(viewId);
    if (trackIdsIt == _tracksPerView->end())
    {
        return 0;
    }

    const auto& trackIds = trackIdsIt->second;

    int count = 0;
    auto descType = aliceVision::feature::EImageDescriberType_stringToEnum(describerType.toStdString());
    for (const auto& trackId : trackIds)
    {
        const auto trackIt = _tracks->find(trackId);
        if (trackIt == _tracks->end()) continue;

        const auto& track = trackIt->second;
        if (track.descType == descType)
        {
            ++count;
        }
    }

    return count;
}

} // namespace qtAliceVision

#include "MTracks.moc"
