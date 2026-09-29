#include "MViewStats.hpp"

#include <QThreadPool>
#include <algorithm>
#include <math.h>

namespace qtAliceVision {

void MViewStats::computeViewStats()
{
    _residualHistogramFull = aliceVision::utils::Histogram<double>();
    _residualHistogramView = aliceVision::utils::Histogram<double>();

    _observationsLengthsHistogramFull = aliceVision::utils::Histogram<double>();
    _observationsLengthsHistogramView = aliceVision::utils::Histogram<double>();

    _observationsScaleHistogramFull = aliceVision::utils::Histogram<double>();
    _observationsScaleHistogramView = aliceVision::utils::Histogram<double>();

    if (_msfmData == nullptr)
    {
        qInfo() << "[QtAliceVision]  MViewStats::computeViewStats: no SfMData loaded";
        return;
    }
    if (_viewId == aliceVision::UndefinedIndexT)
    {
        qInfo() << "[QtAliceVision]  MViewStats::computeViewStats: no valid view: " << _viewId;
        return;
    }
    using namespace aliceVision;
    // residual histogram
    {
        // Init max values per axis
        _residualMaxAxisX = 0.0;
        _residualMaxAxisY = 0.0;

        {
            // residual histogram of all views
            BoxStats<double> residualFullStats;
            sfm::computeResidualsHistogram(_msfmData->rawData(), residualFullStats, &_residualHistogramFull);

            double nbCameras = double(_msfmData->nbCameras());

            // normalize the histogram to get the average number of observations
            std::vector<size_t>& residualFullHistY = _residualHistogramFull.GetHist();
            for (std::size_t i = 0; i < residualFullHistY.size(); ++i)
            {
                residualFullHistY[i] = static_cast<size_t>(round(static_cast<double>(residualFullHistY[i]) / nbCameras));
            }
            std::vector<double> residualHistX = _residualHistogramFull.GetXbinsValue();

            for (std::size_t i = 0; i < residualHistX.size(); ++i)
            {
                _residualMaxAxisX = round(std::max(_residualMaxAxisX, residualHistX[i]));
            }
            for (std::size_t i = 0; i < residualFullHistY.size(); ++i)
            {
                _residualMaxAxisY = round(std::max(_residualMaxAxisY, double(residualFullHistY[i])));
            }
        }
        {
            // residual histogram of current view
            BoxStats<double> residualViewStats;
            sfm::computeResidualsHistogram(_msfmData->rawData(), residualViewStats, &_residualHistogramView, {_viewId});
            std::vector<size_t>& residualViewHistY = _residualHistogramView.GetHist();
            std::vector<double> residualHistX = _residualHistogramView.GetXbinsValue();

            for (std::size_t i = 0; i < residualHistX.size(); ++i)
            {
                _residualMaxAxisX = round(std::max(_residualMaxAxisX, residualHistX[i]));
            }
            for (std::size_t i = 0; i < residualViewHistY.size(); ++i)
            {
                _residualMaxAxisY = round(std::max(_residualMaxAxisY, double(residualViewHistY[i])));
            }
        }
    }

    _nbObservations = 0;
    {
        _observationsLengthsMaxAxisX = 0.0;
        _observationsLengthsMaxAxisY = 0.0;
        // observationsLengths histogram
        {
            // observationsLengths histogram of all views
            BoxStats<double> observationsLengthsFullStats;
            sfm::computeObservationsLengthsHistogram(
              _msfmData->rawData(), observationsLengthsFullStats, _nbObservations, &_observationsLengthsHistogramFull);

            const double nbCameras = double(_msfmData->nbCameras());
            std::vector<size_t>& observationsLengthsFullHistY = _observationsLengthsHistogramFull.GetHist();

            // normalize the histogram to get the average number of observations
            for (std::size_t i = 0; i < observationsLengthsFullHistY.size(); ++i)
            {
                observationsLengthsFullHistY[i] = static_cast<size_t>(round(static_cast<double>(observationsLengthsFullHistY[i]) / nbCameras));
            }
            std::vector<double> observationsLengthsHistX = _observationsLengthsHistogramFull.GetXbinsValue();

            for (std::size_t i = 0; i < observationsLengthsHistX.size(); ++i)
            {
                _observationsLengthsMaxAxisX = round(std::max(_observationsLengthsMaxAxisX, observationsLengthsHistX[i]));
            }
            for (std::size_t i = 0; i < observationsLengthsFullHistY.size(); ++i)
            {
                _observationsLengthsMaxAxisY = round(std::max(_observationsLengthsMaxAxisY, double(observationsLengthsFullHistY[i])));
            }
        }
        {
            // observationsLengths histogram of current view
            BoxStats<double> observationsLengthsViewStats;
            sfm::computeObservationsLengthsHistogram(
              _msfmData->rawData(), observationsLengthsViewStats, _nbObservations, &_observationsLengthsHistogramView, {_viewId});
            const std::vector<size_t> observationsLengthsViewHistY = _observationsLengthsHistogramView.GetHist();
            const std::vector<double> observationsLengthsHistX = _observationsLengthsHistogramView.GetXbinsValue();

            for (std::size_t i = 0; i < observationsLengthsHistX.size(); ++i)
            {
                _observationsLengthsMaxAxisX = round(std::max(_observationsLengthsMaxAxisX, observationsLengthsHistX[i]));
            }
            for (std::size_t i = 0; i < observationsLengthsViewHistY.size(); ++i)
            {
                _observationsLengthsMaxAxisY = round(std::max(_observationsLengthsMaxAxisY, double(observationsLengthsViewHistY[i])));
            }
        }
    }

    // scale histogram
    {
        // histogram of observations Scale of all views
        BoxStats<double> observationsScaleFullStats;
        sfm::computeScaleHistogram(_msfmData->rawData(), observationsScaleFullStats, &_observationsScaleHistogramFull);

        const double nbCameras = double(_msfmData->nbCameras());

        // normalize the histogram to get the average number of observations
        std::vector<size_t>& observationsScaleFullHistY = _observationsScaleHistogramFull.GetHist();
        for (std::size_t i = 0; i < observationsScaleFullHistY.size(); ++i)
        {
            observationsScaleFullHistY[i] = static_cast<std::size_t>(std::round(static_cast<double>(observationsScaleFullHistY[i]) / nbCameras));
        }
        std::vector<double> observationsScaleHistX = _observationsScaleHistogramFull.GetXbinsValue();

        // histogram of observations Scale of current view
        BoxStats<double> observationsScaleViewStats;
        sfm::computeScaleHistogram(_msfmData->rawData(), observationsScaleViewStats, &_observationsScaleHistogramView, {_viewId});
        const std::vector<size_t> observationsScaleViewHistY = _observationsScaleHistogramView.GetHist();

        _observationsScaleMaxAxisX = 0.0;
        _observationsScaleMaxAxisY = 0.0;

        for (std::size_t i = 0; i < observationsScaleHistX.size(); ++i)
        {
            _observationsScaleMaxAxisX = round(std::max(_observationsScaleMaxAxisX, observationsScaleHistX[i]));
        }
        for (std::size_t i = 0; i < observationsScaleFullHistY.size(); ++i)
        {
            _observationsScaleMaxAxisY = round(std::max(_observationsScaleMaxAxisY, double(observationsScaleFullHistY[i])));
        }
        for (std::size_t i = 0; i < observationsScaleViewHistY.size(); ++i)
        {
            _observationsScaleMaxAxisY = round(std::max(_observationsScaleMaxAxisY, double(observationsScaleViewHistY[i])));
        }
    }

    Q_EMIT viewStatsChanged();
}

void MViewStats::setMSfmData(qtAliceVision::MSfMData* sfmData)
{
    if (_msfmData != nullptr)
    {
        disconnect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    _msfmData = sfmData;
    if (_msfmData != nullptr)
    {
        connect(_msfmData, SIGNAL(sfmDataChanged()), this, SIGNAL(sfmDataChanged()));
    }
    Q_EMIT sfmDataChanged();
}

QVariantList MViewStats::getResidualFullPoints()
{
    QVariantList points;

    if (_msfmData == nullptr || _viewId == aliceVision::UndefinedIndexT)
        return points;

    std::vector<double> residualHistX = _residualHistogramFull.GetXbinsValue();
    std::vector<size_t> residualHistY = _residualHistogramFull.GetHist();

    if (residualHistX.size() != residualHistY.size())
        throw std::runtime_error("MViewStats::getResidualFullPoints: residualHistX & residualHistY size mismatch.");

    points.reserve(static_cast<int>(residualHistX.size()));
    for (std::size_t i = 0; i < residualHistX.size(); ++i)
        points.push_back(QPointF(residualHistX[i], double(residualHistY[i])));

    return points;
}

QVariantList MViewStats::getResidualViewPoints()
{
    QVariantList points;

    if (_msfmData == nullptr || _viewId == aliceVision::UndefinedIndexT)
        return points;

    std::vector<double> residualHistX = _residualHistogramView.GetXbinsValue();
    std::vector<size_t> residualHistY = _residualHistogramView.GetHist();

    if (residualHistX.size() != residualHistY.size())
        throw std::runtime_error("MViewStats::getResidualViewPoints: residualHistX & residualHistY size mismatch.");

    points.reserve(static_cast<int>(residualHistX.size()));
    for (std::size_t i = 0; i < residualHistX.size(); ++i)
        points.push_back(QPointF(residualHistX[i], double(residualHistY[i])));

    return points;
}

QVariantList MViewStats::getObservationsLengthsFullPoints()
{
    QVariantList points;

    if (_msfmData == nullptr || _viewId == aliceVision::UndefinedIndexT)
        return points;

    std::vector<double> histX = _observationsLengthsHistogramFull.GetXbinsValue();
    std::vector<size_t> histY = _observationsLengthsHistogramFull.GetHist();

    if (histX.size() != histY.size())
        throw std::runtime_error("MViewStats::getObservationsLengthsFullPoints: histX & histY size mismatch.");

    points.reserve(static_cast<int>(histX.size()));
    for (std::size_t i = 0; i < histX.size(); ++i)
        points.push_back(QPointF(histX[i], double(histY[i])));

    return points;
}

QVariantList MViewStats::getObservationsLengthsViewPoints()
{
    QVariantList points;

    if (_msfmData == nullptr || _viewId == aliceVision::UndefinedIndexT)
        return points;

    std::vector<double> histX = _observationsLengthsHistogramView.GetXbinsValue();
    std::vector<size_t> histY = _observationsLengthsHistogramView.GetHist();

    if (histX.size() != histY.size())
        throw std::runtime_error("MViewStats::getObservationsLengthsViewPoints: histX & histY size mismatch.");

    points.reserve(static_cast<int>(histX.size()));
    for (std::size_t i = 0; i < histX.size(); ++i)
        points.push_back(QPointF(histX[i], double(histY[i])));

    return points;
}

QVariantList MViewStats::getObservationsScaleFullPoints()
{
    QVariantList points;

    if (_msfmData == nullptr || _viewId == aliceVision::UndefinedIndexT)
        return points;

    std::vector<double> histX = _observationsScaleHistogramFull.GetXbinsValue();
    std::vector<size_t> histY = _observationsScaleHistogramFull.GetHist();

    if (histX.size() != histY.size())
        throw std::runtime_error("MViewStats::getObservationsScaleFullPoints: histX & histY size mismatch.");

    points.reserve(static_cast<int>(histX.size()));
    for (std::size_t i = 0; i < histX.size(); ++i)
        points.push_back(QPointF(histX[i], double(histY[i])));

    return points;
}

QVariantList MViewStats::getObservationsScaleViewPoints()
{
    QVariantList points;

    if (_msfmData == nullptr || _viewId == aliceVision::UndefinedIndexT)
        return points;

    std::vector<double> histX = _observationsScaleHistogramView.GetXbinsValue();
    std::vector<size_t> histY = _observationsScaleHistogramView.GetHist();

    if (histX.size() != histY.size())
        throw std::runtime_error("MViewStats::getObservationsScaleViewPoints: histX & histY size mismatch.");

    points.reserve(static_cast<int>(histX.size()));
    for (std::size_t i = 0; i < histX.size(); ++i)
        points.push_back(QPointF(histX[i], double(histY[i])));

    return points;
}

}  // namespace qtAliceVision
