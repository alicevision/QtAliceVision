#include "MSfMDataStats.hpp"

#include <QThreadPool>

#include <ctime>

namespace qtAliceVision
{

void MSfMDataStats::fillLandmarksPerViewSerie(QXYSeries* landmarksPerView)
{
    if (landmarksPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillLandmarksPerViewSerie: no landmarksPerView";
        return;
    }
    landmarksPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillLandmarksPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbLandmarksPerView.size(); ++i)
    {
        landmarksPerView->append(double(i), double(_nbLandmarksPerView[i]));
    }
}

void MSfMDataStats::fillTracksPerViewSerie(QXYSeries* tracksPerView)
{
    if (tracksPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillTracksPerViewSerie: no tracksPerView";
        return;
    }
    tracksPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillTracksPerViewSerie: no SfMData loaded";
        return;
    }

    if (_mTracks == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillTracksPerViewSerie: no Tracks loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbTracksPerView.size(); ++i)
    {
        tracksPerView->append(double(i), double(_nbTracksPerView[i]));
    }
}

void MSfMDataStats::fillResidualsMinPerViewSerie(QXYSeries* residualsPerView)
{
    if (residualsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbResidualsPerViewMin.size(); ++i)
    {
        residualsPerView->append(double(i), double(_nbResidualsPerViewMin[i]));
    }
}

void MSfMDataStats::fillResidualsMaxPerViewSerie(QXYSeries* residualsPerView)
{
    if (residualsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbResidualsPerViewMax.size(); ++i)
    {
        residualsPerView->append(double(i), double(_nbResidualsPerViewMax[i]));
    }
}

void MSfMDataStats::fillResidualsMeanPerViewSerie(QXYSeries* residualsPerView)
{
    if (residualsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbResidualsPerViewMean.size(); ++i)
    {
        residualsPerView->append(double(i), double(_nbResidualsPerViewMean[i]));
    }
}

void MSfMDataStats::fillResidualsMedianPerViewSerie(QXYSeries* residualsPerView)
{
    if (residualsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbResidualsPerViewMedian.size(); ++i)
    {
        residualsPerView->append(double(i), double(_nbResidualsPerViewMedian[i]));
    }
}

void MSfMDataStats::fillResidualsFirstQuartilePerViewSerie(QXYSeries* residualsPerView)
{
    if (residualsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsFirstQuartilePerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsFirstQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbResidualsPerViewFirstQuartile.size(); ++i)
    {
        residualsPerView->append(double(i), double(_nbResidualsPerViewFirstQuartile[i]));
    }
}

void MSfMDataStats::fillResidualsThirdQuartilePerViewSerie(QXYSeries* residualsPerView)
{
    if (residualsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsThirdQuartilePerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillResidualsThirdQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbResidualsPerViewThirdQuartile.size(); ++i)
    {
        residualsPerView->append(double(i), double(_nbResidualsPerViewThirdQuartile[i]));
    }
}

void MSfMDataStats::fillObservationsLengthsMinPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if (observationsLengthsPerView == nullptr)
    {
        qInfo()
            << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMinPerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMinPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbObservationsLengthsPerViewMin.size(); ++i)
    {
        observationsLengthsPerView->append(double(i), double(_nbObservationsLengthsPerViewMin[i]));
    }
}

void MSfMDataStats::fillObservationsLengthsMaxPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if (observationsLengthsPerView == nullptr)
    {
        qInfo()
            << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMaxPerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMaxPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbObservationsLengthsPerViewMax.size(); ++i)
    {
        observationsLengthsPerView->append(double(i), double(_nbObservationsLengthsPerViewMax[i]));
    }
}

void MSfMDataStats::fillObservationsLengthsMeanPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if (observationsLengthsPerView == nullptr)
    {
        qInfo()
            << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMeanPerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMeanPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbObservationsLengthsPerViewMean.size(); ++i)
    {
        observationsLengthsPerView->append(double(i), double(_nbObservationsLengthsPerViewMean[i]));
    }
}

void MSfMDataStats::fillObservationsLengthsMedianPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if (observationsLengthsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMedianPerViewSerie: no "
                   "observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMedianPerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbObservationsLengthsPerViewMedian.size(); ++i)
    {
        observationsLengthsPerView->append(double(i), double(_nbObservationsLengthsPerViewMedian[i]));
    }
}

void MSfMDataStats::fillObservationsLengthsFirstQuartilePerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if (observationsLengthsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsFirstQuartilePerViewSerie: no "
                   "observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsFirstQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbObservationsLengthsPerViewFirstQuartile.size(); ++i)
    {
        observationsLengthsPerView->append(double(i), double(_nbObservationsLengthsPerViewFirstQuartile[i]));
    }
}

void MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if (observationsLengthsPerView == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: no "
                   "observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for (std::size_t i = 0; i < _nbObservationsLengthsPerViewThirdQuartile.size(); ++i)
    {
        observationsLengthsPerView->append(double(i), double(_nbObservationsLengthsPerViewThirdQuartile[i]));
    }
}

void MSfMDataStats::computeGlobalSfMStats()
{
    using namespace aliceVision;
    _nbResidualsPerViewMin.clear();
    _nbResidualsPerViewMax.clear();
    _nbResidualsPerViewMean.clear();
    _nbResidualsPerViewMedian.clear();
    _nbResidualsPerViewFirstQuartile.clear();
    _nbResidualsPerViewThirdQuartile.clear();
    _nbObservationsLengthsPerViewMin.clear();
    _nbObservationsLengthsPerViewMax.clear();
    _nbObservationsLengthsPerViewMean.clear();
    _nbObservationsLengthsPerViewMedian.clear();
    _nbObservationsLengthsPerViewFirstQuartile.clear();
    _nbObservationsLengthsPerViewThirdQuartile.clear();
    _nbLandmarksPerView.clear();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalStats: no SfMData";
        return;
    }
    if (_msfmData->status() != MSfMData::Ready)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalStats: SfMData is not ready: " << _msfmData->status();
        return;
    }
    if (_msfmData->rawData().getViews().empty())
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalStats: SfMData is empty";
        return;
    }

    // Landmarks per View
    {
        std::vector<int> nbLandmarksPerView;
        sfm::computeLandmarksPerView(_msfmData->rawData(), nbLandmarksPerView);

        _nbLandmarksPerView.resize(nbLandmarksPerView.size());
        std::copy(nbLandmarksPerView.begin(), nbLandmarksPerView.end(), _nbLandmarksPerView.begin());

        _landmarksPerViewMaxAxisX = static_cast<int>(_msfmData->rawData().getViews().size());
        _landmarksPerViewMaxAxisY = 0.0;

        for (int v : nbLandmarksPerView)
        {
            _landmarksPerViewMaxAxisY = std::max(_landmarksPerViewMaxAxisY, double(v));
        }
    }

    // Collect residuals histogram for each view

    // Residuals Per View graph
    {
        _residualsPerViewMaxAxisX = 0;
        sfm::computeResidualsPerView(_msfmData->rawData(), _residualsPerViewMaxAxisX, _nbResidualsPerViewMin,
                                     _nbResidualsPerViewMax, _nbResidualsPerViewMean, _nbResidualsPerViewMedian,
                                     _nbResidualsPerViewFirstQuartile, _nbResidualsPerViewThirdQuartile);
        _residualsPerViewMaxAxisY = 0.0;

        for (std::size_t i = 0; i < _nbResidualsPerViewMin.size(); ++i)
        {
            _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMin[i])));
            _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMax[i])));
            _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMean[i])));
            _residualsPerViewMaxAxisY =
                round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMedian[i])));
            _residualsPerViewMaxAxisY =
                round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewFirstQuartile[i])));
            _residualsPerViewMaxAxisY =
                round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewThirdQuartile[i])));
        }
    }

    // Collect observations lengths histogram for each view

    // Observations Lengths Per View graph
    {
        _observationsLengthsPerViewMaxAxisX = 0;
        sfm::computeObservationsLengthsPerView(
            _msfmData->rawData(), _observationsLengthsPerViewMaxAxisX, _nbObservationsLengthsPerViewMin,
            _nbObservationsLengthsPerViewMax, _nbObservationsLengthsPerViewMean, _nbObservationsLengthsPerViewMedian,
            _nbObservationsLengthsPerViewFirstQuartile, _nbObservationsLengthsPerViewThirdQuartile);
        _observationsLengthsPerViewMaxAxisY = 0.0;

        for (std::size_t i = 0; i < _nbObservationsLengthsPerViewMin.size(); ++i)
        {
            _observationsLengthsPerViewMaxAxisY =
                round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMin[i])));
            _observationsLengthsPerViewMaxAxisY =
                round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMax[i])));
            _observationsLengthsPerViewMaxAxisY =
                round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMean[i])));
            _observationsLengthsPerViewMaxAxisY =
                round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMedian[i])));
            _observationsLengthsPerViewMaxAxisY = round(
                std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewFirstQuartile[i])));
            _observationsLengthsPerViewMaxAxisY = round(
                std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewThirdQuartile[i])));
        }
    }

    Q_EMIT sfmStatsChanged();
}

void MSfMDataStats::computeGlobalTracksStats()
{
    _nbTracksPerView.clear();

    // Tracks per View

    if (_mTracks == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalTracksStats: no Tracks loaded";
        return;
    }
    if (_mTracks->status() != MTracks::Ready)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalTracksStats: Tracks is not ready: "
                << _mTracks->status();
        return;
    }
    if (_mTracks->tracks().empty())
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalTracksStats: Tracks is empty";
        return;
    }
    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalTracksStats: no SfMData";
        return;
    }
    if (_msfmData->status() != MSfMData::Ready)
    {
        qInfo() << "[QtAliceVision] MSfMDataStats::computeGlobalTracksStats: SfMData is not ready: "
                << _msfmData->status();
        return;
    }

    _nbTracksPerView.reserve(_msfmData->rawData().getViews().size());
    for (const auto& viewIt : _msfmData->rawData().getViews())
    {
        const auto viewId = viewIt.first;

        if (_mTracks->tracksPerView().count(viewId))
            _nbTracksPerView.push_back(double(_mTracks->tracksPerView().at(viewId).size()));
        else
            _nbTracksPerView.push_back(0.0);
    }

    for (double v : _nbTracksPerView)
    {
        _landmarksPerViewMaxAxisY = std::max(_landmarksPerViewMaxAxisY, v);
    }

    Q_EMIT tracksStatsChanged();
}

MSfMDataStats::~MSfMDataStats()
{
    setMSfmData(nullptr);
    setMTracks(nullptr);
}

void MSfMDataStats::setMSfmData(qtAliceVision::MSfMData* sfmData)
{
    if (_msfmData == sfmData)
    {
        qInfo() << "[QtAliceVision]  MSfMDataStats::setMSfMData: Reset the same pointer";
        return;
    }
    if (_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;
    if (_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    if (_msfmData == nullptr)
    {
        Q_EMIT sfmDataChanged();
    }
    else if (_msfmData->status() == MSfMData::Ready)
    {
        Q_EMIT sfmDataChanged();
    }
}

void MSfMDataStats::setMTracks(qtAliceVision::MTracks* tracks)
{
    if (_mTracks == tracks)
    {
        qInfo() << "[QtAliceVision]  MSfMDataStats::setMTracks: Reset the same pointer";
        return;
    }
    if (_mTracks != nullptr)
    {
        disconnect(_mTracks, SIGNAL(tracksChanged()), this, SIGNAL(tracksChanged()));
    }
    _mTracks = tracks;
    if (_mTracks != nullptr)
    {
        connect(_mTracks, SIGNAL(tracksChanged()), this, SIGNAL(tracksChanged()));
    }
    Q_EMIT tracksChanged();
}

} // namespace qtAliceVision
