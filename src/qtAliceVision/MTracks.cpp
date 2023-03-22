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
    using namespace aliceVision;

    std::unique_ptr<track::TracksMap> tracks(new track::TracksMap());
    std::unique_ptr<track::TracksPerView> tracksPerView(new track::TracksPerView());
    try
    {
        matching::PairwiseMatches pairwiseMatches;
        if (!matching::Load(pairwiseMatches,
                            /*viewsKeysFilter=*/{}, {_matchingFolder.toLocalFile().toStdString()},
                            /*descTypes=*/{},
                            /*maxNbMatches=*/0,
                            /*minNbMatches=*/0))
        {
            qDebug() << "[QtAliceVision] Failed to load matches: " << _matchingFolder << ".";
        }
        track::TracksBuilder tracksBuilder;
        tracksBuilder.build(pairwiseMatches);
        tracksBuilder.exportToSTL(*tracks);
        track::computeTracksPerView(*tracks, *tracksPerView);
    }
    catch (std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load matches: " << _matchingFolder << "."
                 << "\n"
                 << e.what();
    }

    Q_EMIT resultReady(tracks.release(), tracksPerView.release());
}

MTracks::MTracks()
{
    connect(this, &MTracks::matchingFolderChanged, this, &MTracks::load);
}

MTracks::~MTracks()
{
    setStatus(None);
}

void MTracks::clear()
{
    if (_tracks)
        _tracks->clear();
    if (_tracksPerView)
        _tracksPerView->clear();
    qInfo() << "[QtAliceVision] MTracks clear";
    Q_EMIT tracksChanged();
}

void MTracks::load()
{
    qDebug() << "MTracks::load _matchingFolder: " << _matchingFolder;
    if (_matchingFolder.isEmpty())
    {
        setStatus(None);
        clear();
        return;
    }
    if (!QFileInfo::exists(_matchingFolder.toLocalFile()))
    {
        setStatus(Error);
        clear();
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
    setStatus(Loading);
    _tracks.reset(tracks);
    _tracksPerView.reset(tracksPerView);
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
