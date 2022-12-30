#pragma once

#include <QtCharts/QLineSeries>
#include <QQuickItem>
#include <QObject>
#include <QRunnable>
#include <QUrl>
#include <QList>
#include <QPointF>

#include <MSfMData.hpp>
#include <MFeature.hpp>
#include <aliceVision/sfm/sfmStatistics.hpp>
#include <aliceVision/utils/Histogram.hpp>


namespace qtAliceVision {

QT_CHARTS_USE_NAMESPACE

class MViewStats : public QObject
{
    Q_OBJECT
    /// ViewID to consider
    Q_PROPERTY(quint32 viewId MEMBER _viewId NOTIFY viewIdChanged)
    /// Pointer to sfmData
    Q_PROPERTY(qtAliceVision::MSfMData* msfmData READ getMSfmData WRITE setMSfmData NOTIFY sfmDataChanged)
    /// max AxisX value for reprojection residual histogram
    Q_PROPERTY(double residualMaxAxisX MEMBER _residualMaxAxisX NOTIFY viewStatsChanged)
    /// max AxisY value for reprojection residual histogram
    Q_PROPERTY(double residualMaxAxisY MEMBER _residualMaxAxisY NOTIFY viewStatsChanged)
    /// max AxisX value for observations lengths histogram
    Q_PROPERTY(double observationsLengthsMaxAxisX MEMBER _observationsLengthsMaxAxisX NOTIFY viewStatsChanged)
    /// max AxisY value for observations lengths histogram
    Q_PROPERTY(double observationsLengthsMaxAxisY MEMBER _observationsLengthsMaxAxisY NOTIFY viewStatsChanged)
    /// max AxisX value for scale histogram
    Q_PROPERTY(double observationsScaleMaxAxisX MEMBER _observationsScaleMaxAxisX NOTIFY viewStatsChanged)
    /// max AxisY value for scale histogram
    Q_PROPERTY(double observationsScaleMaxAxisY MEMBER _observationsScaleMaxAxisY NOTIFY viewStatsChanged)

public:
    MViewStats()
    {
         connect(this, &MViewStats::sfmDataChanged, this, &MViewStats::computeViewStats);
         connect(this, &MViewStats::viewIdChanged, this, &MViewStats::computeViewStats);
    }
    MViewStats& operator=(const MViewStats& other) = default;
    ~MViewStats() override = default;

    Q_SIGNAL void sfmDataChanged();
    Q_SIGNAL void viewIdChanged();
    Q_SIGNAL void viewStatsChanged();

    Q_INVOKABLE void fillResidualFullSerie(QXYSeries* serie);
    Q_INVOKABLE void fillResidualViewSerie(QXYSeries* serie);

    Q_INVOKABLE void fillObservationsLengthsFullSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsLengthsViewSerie(QXYSeries* serie);

    Q_INVOKABLE void fillObservationsScaleFullSerie(QXYSeries* serie);
    Q_INVOKABLE void fillObservationsScaleViewSerie(QXYSeries* serie);

    Q_SLOT void computeViewStats();

    MSfMData* getMSfmData() { return _msfmData; }
    void setMSfmData(qtAliceVision::MSfMData* sfmData);


private:
    aliceVision::utils::Histogram<double> _residualHistogramFull;
    aliceVision::utils::Histogram<double> _residualHistogramView;
    aliceVision::utils::Histogram<double> _observationsLengthsHistogramFull;
    aliceVision::utils::Histogram<double> _observationsLengthsHistogramView;
    aliceVision::utils::Histogram<double> _observationsScaleHistogramFull;
    aliceVision::utils::Histogram<double> _observationsScaleHistogramView;
    double _residualMaxAxisX = 0.0;
    double _residualMaxAxisY = 0.0;
    double _observationsLengthsMaxAxisX = 0.0;
    double _observationsLengthsMaxAxisY = 0.0;
    double _observationsScaleMaxAxisX = 0.0;
    double _observationsScaleMaxAxisY = 0.0;
    int _nbObservations = 0;
    MSfMData* _msfmData = nullptr;
    aliceVision::IndexT _viewId = aliceVision::UndefinedIndexT;
};

}

Q_DECLARE_METATYPE(QPointF)   // for usage in signals/slots/properties
