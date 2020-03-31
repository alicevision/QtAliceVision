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

    std::vector<double> landmarksPerViewHistX = _landmarksPerViewHistogram.GetXbinsValue();
    std::vector<size_t> landmarksPerViewHistY = _landmarksPerViewHistogram.GetHist();
    assert(landmarksPerViewHistX.size() == landmarksPerViewHistY.size());
    for(std::size_t i = 0; i < landmarksPerViewHistX.size(); ++i)
    {
        landmarksPerView->append(double(landmarksPerViewHistX[i]), double(landmarksPerViewHistY[i]));
    }

    qWarning() << "[QtAliceVision] MSfMDataStats::fillLandmarksPerViewSerie: landmarksPerView size (" << landmarksPerView->count() << ")";

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

void MSfMDataStats::fillPointsValidatedPerViewSerie(QXYSeries* pointsValidatedPerView)
{
    if(pointsValidatedPerView == nullptr)
    {
        qWarning() << "[QtAliceVision] MSfMDataStats::fillPointsValidatedPerViewSerie: no pointsValidatedPerView";
        return;
    }
    pointsValidatedPerView->clear();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision]  MSfMDataStats::fillPointsValidatedPerViewSerie: no SfMData loaded";
        return;
    }

    for(int i = 0; i < _pointsValidatedPerViewMaxAxisX; ++i)
    {
        //pointsValidatedPerView->append(i, double(_nbResidualsPerViewMedian[i]));
    }

    qWarning() << "[QtAliceVision]  MSfMDataStats::fillPointsValidatedPerViewSerie: pointsValidatedPerView size (" << pointsValidatedPerView->count() << ")";

}

void MSfMDataStats::computeGlobalStats()
{

     using namespace aliceVision;
    _landmarksPerViewHistogram = aliceVision::Histogram<double>();

    if(_msfmData == nullptr)
    {
        qWarning() << "[QtAliceVision]  MSfMDataStats::computeGlobalStats: no SfMData loaded";
        return;
    }

    // Init tracksPerView to have an entry in the map for each view (even if there is no track at all)
    for(const auto& viewIt: _msfmData->rawData().getViewsKeys())
    {
        qWarning() << "[QtAliceVision]  MSfMDataStats::computeGlobalStats: viewIt: " << viewIt;

        // create an entry in the map
        _tracksPerView[viewIt];
        qWarning() << "[QtAliceVision]  MSfMDataStats::computeGlobalStats: _tracksPerView[viewIt]: " << _tracksPerView[viewIt];
    }

    //Pb: access to index value of tracksPerView in the compute function

    //Landmarks per View histogram
    {
        MinMaxMeanMedian<double> landmarksPerViewStats;
        sfm::computeLandmarksPerViewHistogram(_msfmData->rawData(), landmarksPerViewStats, _tracksPerView, &_landmarksPerViewHistogram);
        _landmarksPerViewMaxAxisX = 0.0;
        _landmarksPerViewMaxAxisY = 0.0;

        std::vector<double> landmarksPerViewHistX = _landmarksPerViewHistogram.GetXbinsValue();
        std::vector<size_t> landmarksPerViewHistY = _landmarksPerViewHistogram.GetHist();
        assert(landmarksPerViewHistX.size() == landmarksPerViewHistY.size());
        for(std::size_t i = 0; i < landmarksPerViewHistX.size(); ++i)
        {
            _landmarksPerViewMaxAxisX = round(std::max(_landmarksPerViewMaxAxisX, landmarksPerViewHistX[i]));
            _landmarksPerViewMaxAxisY = round(std::max(_landmarksPerViewMaxAxisY, double(landmarksPerViewHistY[i])));
        }
    }


   /*   _nbPointsValidatedPerView.reserve(_msfmData->rawData().getViews().size());

      //points validated per View histogram
      {
          _pointsValidatedPerViewMaxAxisX = 0.0;
          sfm::computePointsValidatedPerView(_msfmData->rawData(), _pointsValidatedPerViewMaxAxisX, _nbPointsValidatedPerView);
          _pointsValidatedPerViewMaxAxisY = 0.0;


          for(int i = 0; i < _pointsValidatedPerViewMaxAxisX; ++i)
          {
              _pointsValidatedPerViewMaxAxisY = round(std::max(_pointsValidatedPerViewMaxAxisY, double(_nbPointsValidatedPerView[i])));
          }
      }*/


    // Collect residuals histogram for each view

    _nbResidualsPerViewMin.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewMax.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewMean.reserve(_msfmData->rawData().getViews().size());
    _nbResidualsPerViewMedian.reserve(_msfmData->rawData().getViews().size());

    //Residuals Per View graph
    {
       _residualsPerViewMaxAxisX = 0;
       sfm::computeResidualsPerView(_msfmData->rawData(), _residualsPerViewMaxAxisX, _nbResidualsPerViewMin, _nbResidualsPerViewMax, _nbResidualsPerViewMean, _nbResidualsPerViewMedian);
       _residualsPerViewMaxAxisY = 0.0;

       for(int i = 0; i < _residualsPerViewMaxAxisX; ++i)
       {
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMin[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMax[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMean[i])));
           _residualsPerViewMaxAxisY = round(std::max(_residualsPerViewMaxAxisY, double(_nbResidualsPerViewMedian[i])));
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
        qWarning() << "[QtAliceVision] MSfMDataStats::setMSfmData disconnect: ";
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;
    if(_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
        qWarning() << "[QtAliceVision] MSfMDataStats::setMSfmData connect: ";
    }
    Q_EMIT sfmDataChanged();
}

}
