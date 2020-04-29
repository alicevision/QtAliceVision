#pragma once

#include <QtCharts/QLineSeries>
#include <QtCharts/QBoxSet>
#include <QObject>
#include <QRunnable>
#include <QUrl>

#include <aliceVision/sfmData/SfMData.hpp>
#include <MSfMData.hpp>
#include <MFeature.hpp>
#include <aliceVision/sfm/sfmStatistics.hpp>
#include <aliceVision/utils/Histogram.hpp>
#include <aliceVision/track/Track.hpp>

namespace qtAliceVision {

QT_CHARTS_USE_NAMESPACE

class MSfMDataStats: public QObject
{
    Q_OBJECT

    /// Pointer to sfmData
    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)
    /// max AxisX value for landmarks per view histogram
    Q_PROPERTY(int landmarksPerViewMaxAxisX MEMBER _landmarksPerViewMaxAxisX NOTIFY statsChanged)
    /// max AxisY value for landmarks per view histogram
    Q_PROPERTY(double landmarksPerViewMaxAxisY MEMBER _landmarksPerViewMaxAxisY NOTIFY statsChanged)
    /// max AxisX value for residuals per view histogram
    Q_PROPERTY(int residualsPerViewMaxAxisX MEMBER _residualsPerViewMaxAxisX NOTIFY statsChanged)
    /// max AxisY value for residuals per view histogram
    Q_PROPERTY(double residualsPerViewMaxAxisY MEMBER _residualsPerViewMaxAxisY NOTIFY statsChanged)
    /// max AxisX value for residuals per view histogram
    Q_PROPERTY(int observationsLengthsPerViewMaxAxisX MEMBER _observationsLengthsPerViewMaxAxisX NOTIFY statsChanged)
    /// max AxisY value for residuals per view histogram
    Q_PROPERTY(double observationsLengthsPerViewMaxAxisY MEMBER _observationsLengthsPerViewMaxAxisY NOTIFY statsChanged)

public:
    MSfMDataStats()
    {
         connect(this, &MSfMDataStats::sfmDataChanged, this, &MSfMDataStats::computeGlobalStats);
    }
    MSfMDataStats& operator=(const MSfMDataStats& other) = default;
    ~MSfMDataStats() override;

    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void statsChanged();

    Q_SLOT void computeGlobalStats();

    Q_INVOKABLE void fillLandmarksPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillFeaturesPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillTracksPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMinPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMaxPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMeanPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMedianPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsFirstQuartilePerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsThirdQuartilePerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsLengthsMinPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsLengthsMaxPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsLengthsMeanPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsLengthsMedianPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsLengthsFirstQuartilePerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsLengthsThirdQuartilePerViewSerie(QXYSeries* serie);
    //Q_INVOKABLE void fillObservationsLengthsBoxSetPerView(QBoxSet* serie);

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(qtAliceVision::MSfMData* sfmData);

private:
    MSfMData* _msfmData = nullptr;

    int _landmarksPerViewMaxAxisX = 0;
    double _landmarksPerViewMaxAxisY = 0.0;
    int _residualsPerViewMaxAxisX = 0;
    double _residualsPerViewMaxAxisY = 0.0;
    int _observationsLengthsPerViewMaxAxisX = 0;
    double _observationsLengthsPerViewMaxAxisY = 0.0;
    std::vector<double> _nbResidualsPerViewMin;
    std::vector<double> _nbResidualsPerViewMax;
    std::vector<double> _nbResidualsPerViewMean;
    std::vector<double> _nbResidualsPerViewMedian;
    std::vector<double> _nbResidualsPerViewFirstQuartile;
    std::vector<double> _nbResidualsPerViewThirdQuartile;
    std::vector<double> _nbObservationsLengthsPerViewMin;
    std::vector<double> _nbObservationsLengthsPerViewMax;
    std::vector<double> _nbObservationsLengthsPerViewMean;
    std::vector<double> _nbObservationsLengthsPerViewMedian;
    std::vector<double> _nbObservationsLengthsPerViewFirstQuartile;
    std::vector<double> _nbObservationsLengthsPerViewThirdQuartile;
    std::vector<double> _nbLandmarksPerView;
    std::vector<double> _nbFeaturesPerView;
    std::vector<double> _nbTracksPerView;
    aliceVision::track::TracksPerView _tracksPerView;
};

}
