#include "MTracks.hpp"

#include <aliceVision/matching/io.hpp>
#include <aliceVision/track/TracksBuilder.hpp>
#include <aliceVision/track/tracksUtils.hpp>

#include <QThreadPool>
#include <QFileInfo>
#include <QDebug>


namespace qtAliceVision {

/**
 * @brief QRunnable object dedicated to load sfmData using AliceVision.
 */
class TracksIORunnable : public QObject, public QRunnable
{
    Q_OBJECT

public:

    explicit TracksIORunnable(const QUrl& matchingFolder):
    _matchingFolder(matchingFolder)
    {}

    Q_SLOT void run() override;

    Q_SIGNAL void resultReady();

private:
    const QUrl _matchingFolder;

public:
    aliceVision::track::TracksMap _tracks;
    aliceVision::track::TracksPerView _tracksPerView;
};

void TracksIORunnable::run()
{
    using namespace aliceVision;

    try
    {
        matching::PairwiseMatches pairwiseMatches;
        if (!matching::Load(pairwiseMatches,
                            /*viewsKeysFilter=*/{},
                            {_matchingFolder.toLocalFile().toStdString()},
                            /*descTypes=*/{},
                            /*maxNbMatches=*/0,
                            /*minNbMatches=*/0))
        {
            qDebug() << "[QtAliceVision] Failed to load matches: " << _matchingFolder << ".";
        }
        track::TracksBuilder tracksBuilder;
        tracksBuilder.build(pairwiseMatches);
        tracksBuilder.exportToSTL(_tracks);
        track::computeTracksPerView(_tracks, _tracksPerView);
    }
    catch(std::exception& e)
    {
        qDebug() << "[QtAliceVision] Failed to load matches: " << _matchingFolder << "."
                 << "\n" << e.what();
    }

    Q_EMIT resultReady();
}

void MTracks::load()
{
    if(_matchingFolder.isEmpty())
    {
        setStatus(None);
        return;
    }
    if(!QFileInfo::exists(_matchingFolder.toLocalFile()))
    {
        setStatus(Error);
        return;
    }

    setStatus(Loading);    

    // load features from file in a seperate thread
    _ioRunnable = new TracksIORunnable(_matchingFolder);
    _ioRunnable->setAutoDelete(false);
    connect(_ioRunnable, &TracksIORunnable::resultReady, this, &MTracks::onReady);
    QThreadPool::globalInstance()->start(_ioRunnable);
}

void MTracks::onReady()
{
    _tracks = _ioRunnable->_tracks;
    _tracksPerView = _ioRunnable->_tracksPerView;
    _ioRunnable = nullptr;
    setStatus(Ready);
}

}

#include "MTracks.moc"
