#include "MSfMDataStats.hpp"

#include <QThreadPool>


namespace qtAliceVision {

void MSfMDataStats::fillLandmarksPerViewSerie(QXYSeries* landmarksPerView)
{
    if(landmarksPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillLandmarksPerViewSerie: no landmarksPerView";
        return;
    }
    landmarksPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillLandmarksPerViewSerie: no SfMData loaded";
        return;
    }

    qWarning() << "[QtAliceVision]  MSfMDataStats::fillLandmarksPerViewSerie: _nbLandmarksPerView: " << _nbLandmarksPerView.size();

    for(std::size_t i = 0; i < _landmarksPerViewMaxAxisX; ++i)
    {
        landmarksPerView->append(double(i), double(_nbLandmarksPerView[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillLandmarksPerViewSerie: landmarksPerView size (" << landmarksPerView->count() << ")";
}

void MSfMDataStats::fillFeaturesPerViewSerie(QXYSeries* featuresPerView)
{
    if(featuresPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillFeaturesPerViewSerie: no featuresPerView";
        return;
    }
    featuresPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillFeaturesPerViewSerie: no SfMData loaded";
        return;
    }

    qWarning() << "[QtAliceVision]  MSfMDataStats::fillFeaturesPerViewSerie: _nbLandmarksPerView: " << _nbFeaturesPerView.size();

    for(std::size_t i = 0; i < _landmarksPerViewMaxAxisX; ++i)
    {
        featuresPerView->append(double(i), double(_nbFeaturesPerView[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillFeaturesPerViewSerie: featuresPerView size (" << featuresPerView->count() << ")";
}

void MSfMDataStats::fillTracksPerViewSerie(QXYSeries* tracksPerView)
{
    if(tracksPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillTracksPerViewSerie: no tracksPerView";
        return;
    }
    tracksPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillTracksPerViewSerie: no SfMData loaded";
        return;
    }

    for(std::size_t i = 0; i < _landmarksPerViewMaxAxisX; ++i)
    {
        tracksPerView->append(double(i), double(_nbTracksPerView[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillTracksPerViewSerie: tracksPerView size (" << tracksPerView->count() << ")";
}

void MSfMDataStats::fillResidualsMinPerViewSerie(QXYSeries* residualsPerView)
{
    if(residualsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
    {
        residualsPerView->append(i, double(_nbResidualsPerViewMin[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: residualsPerView size (" << residualsPerView->count() << ")";
}

void MSfMDataStats::fillResidualsMaxPerViewSerie(QXYSeries* residualsPerView)
{
    if(residualsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
    {
        residualsPerView->append(i, double(_nbResidualsPerViewMax[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: residualsPerView size (" << residualsPerView->count() << ")";
}

void MSfMDataStats::fillResidualsMeanPerViewSerie(QXYSeries* residualsPerView)
{
    if(residualsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
    {
        residualsPerView->append(i, double(_nbResidualsPerViewMean[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: residualsPerView size (" << residualsPerView->count() << ")";
}

void MSfMDataStats::fillResidualsMedianPerViewSerie(QXYSeries* residualsPerView)
{
    if(residualsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
    {
        residualsPerView->append(i, double(_nbResidualsPerViewMedian[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsPerViewSerie: residualsPerView size (" << residualsPerView->count() << ")";
}

void MSfMDataStats::fillResidualsFirstQuartilePerViewSerie(QXYSeries* residualsPerView)
{
    if(residualsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsFirstQuartilePerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsFirstQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
    {
        residualsPerView->append(i, double(_nbResidualsPerViewFirstQuartile[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsFirstQuartilePerViewSerie: residualsPerView size (" << residualsPerView->count() << ")";
}

void MSfMDataStats::fillResidualsThirdQuartilePerViewSerie(QXYSeries* residualsPerView)
{
    if(residualsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsThirdQuartilePerViewSerie: no residualsPerView";
        return;
    }
    residualsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsThirdQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
    {
        residualsPerView->append(i, double(_nbResidualsPerViewThirdQuartile[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillResidualsThirdQuartilePerViewSerie: residualsPerView size (" << residualsPerView->count() << ")";
}

void MSfMDataStats::fillObservationsLengthsMinPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if(observationsLengthsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMinPerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMinPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
    {
        observationsLengthsPerView->append(i, double(_nbObservationsLengthsPerViewMin[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMinPerViewSerie: observationsLengthsPerView size (" << observationsLengthsPerView->count() << ")";
}

void MSfMDataStats::fillObservationsLengthsMaxPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if(observationsLengthsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMaxPerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMaxPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
    {
        observationsLengthsPerView->append(i, double(_nbObservationsLengthsPerViewMax[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMaxPerViewSerie: observationsLengthsPerView size (" << observationsLengthsPerView->count() << ")";
}

void MSfMDataStats::fillObservationsLengthsMeanPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if(observationsLengthsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMeanPerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMeanPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
    {
        observationsLengthsPerView->append(i, double(_nbObservationsLengthsPerViewMean[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMeanPerViewSerie: observationsLengthsPerView size (" << observationsLengthsPerView->count() << ")";
}

void MSfMDataStats::fillObservationsLengthsMedianPerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if(observationsLengthsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMedianPerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMedianPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
    {
        observationsLengthsPerView->append(i, double(_nbObservationsLengthsPerViewMedian[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsMedianPerViewSerie: observationsLengthsPerView size (" << observationsLengthsPerView->count() << ")";
}

void MSfMDataStats::fillObservationsLengthsFirstQuartilePerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if(observationsLengthsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsFirstQuartilePerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsFirstQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
    {
        observationsLengthsPerView->append(i, double(_nbObservationsLengthsPerViewFirstQuartile[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsFirstQuartilePerViewSerie: observationsLengthsPerView size (" << observationsLengthsPerView->count() << ")";
}

void MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie(QXYSeries* observationsLengthsPerView)
{
    if(observationsLengthsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
    {
        observationsLengthsPerView->append(i, double(_nbObservationsLengthsPerViewThirdQuartile[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: observationsLengthsPerView size (" << observationsLengthsPerView->count() << ")";
}

/*void MSfMDataStats::fillObservationsLengthsBoxSetPerView(QBoxSet* observationsLengthsPerView)
{
    if(observationsLengthsPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: no observationsLengthsPerView";
        return;
    }
    observationsLengthsPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
    {
        observationsLengthsPerView->append();
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillObservationsLengthsThirdQuartilePerViewSerie: observationsLengthsPerView size (" << observationsLengthsPerView->count() << ")";
}*/

void MSfMDataStats::computeGlobalStats()
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
    _nbFeaturesPerView.clear();
    _nbTracksPerView.clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision]  MSfMDataStats::computeGlobalStats: no SfMData loaded";
        return;
    }

    //Landmarks per View
    {
        std::vector<int> nbLandmarksPerView;
        sfm::computeLandmarksPerView(_msfmData->rawData(), nbLandmarksPerView);

        _nbLandmarksPerView.resize(nbLandmarksPerView.size());
        std::copy(nbLandmarksPerView.begin(), nbLandmarksPerView.end(), _nbLandmarksPerView.begin());

        _landmarksPerViewMaxAxisX = round(_msfmData->rawData().getViews().size());
        _landmarksPerViewMaxAxisY = 0.0;

        for(int v: nbLandmarksPerView)
        {
            _landmarksPerViewMaxAxisY = std::max(_landmarksPerViewMaxAxisY, double(v));
        }
    }

    //Features and Tracks per View
    {
        std::vector<size_t> nbFeaturesPerView;
        std::vector<size_t> nbTracksPerView;

        sfm::computeFeatMatchPerView(_msfmData->rawData(), nbFeaturesPerView, nbTracksPerView);

        _nbFeaturesPerView.resize(nbFeaturesPerView.size());
        std::copy(nbFeaturesPerView.begin(), nbFeaturesPerView.end(), _nbFeaturesPerView.begin());
        _nbTracksPerView.resize(nbTracksPerView.size());
        std::copy(nbTracksPerView.begin(), nbTracksPerView.end(), _nbTracksPerView.begin());

        for(int v: nbFeaturesPerView)
        {
            _landmarksPerViewMaxAxisY = std::max(_landmarksPerViewMaxAxisY, double(v));
        }

        for(int v: nbTracksPerView)
        {
            _landmarksPerViewMaxAxisY = std::max(_landmarksPerViewMaxAxisY, double(v));
        }
    }

    // Collect residuals histogram for each view

    _nbResidualsPerViewMin.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewMax.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewMean.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewMedian.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewFirstQuartile.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewThirdQuartile.reserve(_msfmData->rawData().getViews().size());

    //Residuals Per View graph
    {
       _residualsPerViewMaxAxisX = 0;
       sfm::computeResidualsPerView(_msfmData->rawData(), _residualsPerViewMaxAxisX, _nbResidualsPerViewMin, _nbResidualsPerViewMax, _nbResidualsPerViewMean, _nbResidualsPerViewMedian, _nbResidualsPerViewFirstQuartile, _nbResidualsPerViewThirdQuartile);
       _residualsPerViewMaxAxisY = 0.0;

       for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
       {
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMin[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMax[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMean[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMedian[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewFirstQuartile[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewThirdQuartile[i])));
       }
    }


    // Collect observations lengths histogram for each view

    _nbObservationsLengthsPerViewMin.reserve(_msfmData->rawData().getViews().size());
    _nbObservationsLengthsPerViewMax.reserve(_msfmData->rawData().getViews().size());
    _nbObservationsLengthsPerViewMean.reserve(_msfmData->rawData().getViews().size());
    _nbObservationsLengthsPerViewMedian.reserve(_msfmData->rawData().getViews().size());
    _nbObservationsLengthsPerViewFirstQuartile.reserve(_msfmData->rawData().getViews().size());
    _nbObservationsLengthsPerViewThirdQuartile.reserve(_msfmData->rawData().getViews().size());

    //Observations Lengths Per View graph
    {
       _observationsLengthsPerViewMaxAxisX = 0;
       sfm::computeObservationsLengthsPerView(_msfmData->rawData(), _observationsLengthsPerViewMaxAxisX, _nbObservationsLengthsPerViewMin, _nbObservationsLengthsPerViewMax, _nbObservationsLengthsPerViewMean, _nbObservationsLengthsPerViewMedian, _nbObservationsLengthsPerViewFirstQuartile, _nbObservationsLengthsPerViewThirdQuartile);
       _observationsLengthsPerViewMaxAxisY = 0.0;

       for(int i = 0; i < _observationsLengthsPerViewMaxAxisX; ++i)
       {
           _observationsLengthsPerViewMaxAxisY = round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMin[i])));
           _observationsLengthsPerViewMaxAxisY = round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMax[i])));
           _observationsLengthsPerViewMaxAxisY = round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMean[i])));
           _observationsLengthsPerViewMaxAxisY = round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewMedian[i])));
           _observationsLengthsPerViewMaxAxisY = round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewFirstQuartile[i])));
           _observationsLengthsPerViewMaxAxisY = round(std::max(_observationsLengthsPerViewMaxAxisY, double(_nbObservationsLengthsPerViewThirdQuartile[i])));

       }
    }

    Q_EMIT statsChanged();
}

MSfMDataStats::~MSfMDataStats()
{
}

void MSfMDataStats::setMSfmData(qtAliceVision::MSfMData* sfmData)
{
    if(_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;
    if(_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    Q_EMIT sfmDataChanged();
}

}
