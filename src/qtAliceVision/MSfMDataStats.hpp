#pragma once

#include <QObject>
#include <QRunnable>
#include <QUrl>
#include <QtCharts/QBoxSet>
#include <QtCharts/QLineSeries>

#include <aliceVision/sfm/sfmStatistics.hpp>
#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/track/Track.hpp>
#include <aliceVision/utils/Histogram.hpp>
#include <MSfMData.hpp>
#include <MTracks.hpp>

namespace qtAliceVision {

class MSfMDataStats : public QObject
{
    Q_OBJECT

    /// Pointer to sfmData
    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)
    /// Pointer to tracks
    Q_PROPERTY(qtAliceVision::MTracks* mTracks READ getMTracks WRITE setMTracks NOTIFY tracksChanged)
    /// max AxisX value for landmarks per view histogram
    Q_PROPERTY(int landmarksPerViewMaxAxisX MEMBER _landmarksPerViewMaxAxisX NOTIFY axisChanged)
    /// max AxisY value for landmarks per view histogram
    Q_PROPERTY(double landmarksPerViewMaxAxisY MEMBER _landmarksPerViewMaxAxisY NOTIFY axisChanged)
    /// max AxisX value for residuals per view histogram
    Q_PROPERTY(int residualsPerViewMaxAxisX MEMBER _residualsPerViewMaxAxisX NOTIFY axisChanged)
    /// max AxisY value for residuals per view histogram
    Q_PROPERTY(double residualsPerViewMaxAxisY MEMBER _residualsPerViewMaxAxisY NOTIFY axisChanged)
    /// max AxisX value for residuals per view histogram
    Q_PROPERTY(int observationsLengthsPerViewMaxAxisX MEMBER _observationsLengthsPerViewMaxAxisX NOTIFY axisChanged)
    /// max AxisY value for residuals per view histogram
    Q_PROPERTY(double observationsLengthsPerViewMaxAxisY MEMBER _observationsLengthsPerViewMaxAxisY NOTIFY axisChanged)

  public:
    MSfMDataStats()
    {
        connect(this, &MSfMDataStats::sfmDataChanged, this, &MSfMDataStats::computeGlobalSfMStats);
        connect(this, &MSfMDataStats::tracksChanged, this, &MSfMDataStats::computeGlobalTracksStats);
        connect(this, &MSfMDataStats::sfmDataChanged, this, &MSfMDataStats::computeGlobalTracksStats);
        connect(this, &MSfMDataStats::sfmStatsChanged, this, &MSfMDataStats::axisChanged);
        connect(this, &MSfMDataStats::tracksStatsChanged, this, &MSfMDataStats::axisChanged);
    }
    MSfMDataStats& operator=(const MSfMDataStats& other) = default;
    ~MSfMDataStats() override;

    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void tracksChanged();
    Q_SIGNAL void sfmStatsChanged();
    Q_SIGNAL void tracksStatsChanged();
    Q_SIGNAL void axisChanged();

    Q_SLOT void computeGlobalSfMStats();
    Q_SLOT void computeGlobalTracksStats();

    Q_INVOKABLE void fillLandmarksPerViewSerie(QXYSeries* serie);
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

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(qtAliceVision::MSfMData* sfmData);

    MTracks* getMTracks() { return _mTracks; }
    void setMTracks(qtAliceVision::MTracks* tracks);

  private:
    MSfMData* _msfmData = nullptr;
    MTracks* _mTracks = nullptr;

    int _residualsPerViewMaxAxisX = 0;
    int _landmarksPerViewMaxAxisX = 0;
    double _landmarksPerViewMaxAxisY = 0.0;
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
    std::vector<double> _nbTracksPerView;
};

}  // namespace qtAliceVision
