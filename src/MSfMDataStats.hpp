#pragma once

#include <QtCharts/QLineSeries>
#include <QObject>
#include <QRunnable>
#include <QUrl>

#include <aliceVision/sfmData/SfMData.hpp>
#include <MSfMData.hpp>
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
    Q_PROPERTY(int landmarksPerViewMaxAxisY MEMBER _landmarksPerViewMaxAxisY NOTIFY statsChanged)
    /// max AxisX value for points validated per view histogram
    Q_PROPERTY(int pointsValidatedPerViewMaxAxisX MEMBER _pointsValidatedPerViewMaxAxisX NOTIFY statsChanged)
    /// max AxisY value for points validated per view histogram
    Q_PROPERTY(double pointsValidatedPerViewMaxAxisY MEMBER _pointsValidatedPerViewMaxAxisY NOTIFY statsChanged)
    /// max AxisX value for residuals per view histogram
    Q_PROPERTY(int residualsPerViewMaxAxisX MEMBER _residualsPerViewMaxAxisX NOTIFY statsChanged)
    /// max AxisY value for residuals per view histogram
    Q_PROPERTY(double residualsPerViewMaxAxisY MEMBER _residualsPerViewMaxAxisY NOTIFY statsChanged)

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

    Q_INVOKABLE void fillPointsValidatedPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillLandmarksPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMinPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMaxPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMeanPerViewSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualsMedianPerViewSerie(QXYSeries* serie);

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(qtAliceVision::MSfMData* sfmData);

private:
    MSfMData* _msfmData = nullptr;
    aliceVision::Histogram<double> _landmarksPerViewHistogram;
    aliceVision::track::TracksPerView _tracksPerView;
    double _landmarksPerViewMaxAxisX = 0.0;
    double _landmarksPerViewMaxAxisY = 0.0;
    int _pointsValidatedPerViewMaxAxisX = 0;
    double _pointsValidatedPerViewMaxAxisY = 0.0;
    int _residualsPerViewMaxAxisX = 0;
    double _residualsPerViewMaxAxisY = 0.0;
    std::vector<double> _nbResidualsPerViewMin;
    std::vector<double> _nbResidualsPerViewMax;
    std::vector<double> _nbResidualsPerViewMean;
    std::vector<double> _nbResidualsPerViewMedian;
    std::vector<double> _nbPointsValidatedPerView;
};

}
