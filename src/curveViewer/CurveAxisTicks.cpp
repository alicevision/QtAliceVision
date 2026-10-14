#include "CurveAxisTicks.hpp"

#include <algorithm>
#include <cmath>

namespace curveViewer {

namespace {

/// Smallest "nice" step (1, 2 or 5 times a power of 10) greater or equal to rawStep.
double niceStep(double rawStep)
{
    const double magnitude = std::pow(10.0, std::floor(std::log10(rawStep)));
    const double normalized = rawStep / magnitude;
    if (normalized <= 1.0)
    {
        return magnitude;
    }
    if (normalized <= 2.0)
    {
        return 2.0 * magnitude;
    }
    if (normalized <= 5.0)
    {
        return 5.0 * magnitude;
    }
    return 10.0 * magnitude;
}

}  // namespace

CurveAxisTicks::CurveAxisTicks(QObject* parent)
  : QAbstractListModel(parent)
{}

int CurveAxisTicks::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(_ticks.size());
}

QVariant CurveAxisTicks::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
    {
        return {};
    }

    const Tick& t = _ticks[static_cast<std::size_t>(index.row())];
    switch (role)
    {
        case PositionRole:
            return t.position;
        case LabelRole:
            return t.label;
        default:
            return {};
    }
}

QHash<int, QByteArray> CurveAxisTicks::roleNames() const
{
    return {
      {PositionRole, "position"},
      {LabelRole, "label"},
    };
}

void CurveAxisTicks::update(double min, double max, double pixelLength, double minSpacing, bool inverted)
{
    std::vector<Tick> ticks;
    double step = _step;

    const double range = max - min;
    if (pixelLength > 1.0 && range > 0.0 && std::isfinite(range))
    {
        step = niceStep(range * minSpacing / pixelLength);
        const int decimals = std::max(0, static_cast<int>(-std::floor(std::log10(step) + 1e-9)));
        const double first = std::ceil(min / step);
        const double last = std::floor(max / step);
        const double scale = pixelLength / range;
        // Integer counter, bounded by the number of ticks that fit: a double counter would stop
        // incrementing when min / step exceeds 2^53
        const double count = std::min(last - first + 1.0, std::floor(pixelLength / minSpacing) + 2.0);
        for (int i = 0; i < static_cast<int>(count); ++i)
        {
            double value = (first + i) * step;
            if (std::abs(value) < step * 1e-6)
            {
                value = 0.0;  // avoid "-0"
            }
            const double offset = (value - min) * scale;
            ticks.push_back({inverted ? pixelLength - offset : offset, QString::number(value, 'f', decimals)});
        }
    }

    if (ticks.size() == _ticks.size())
    {
        _ticks = std::move(ticks);
        if (!_ticks.empty())
        {
            Q_EMIT dataChanged(index(0), index(rowCount() - 1));
        }
    }
    else
    {
        beginResetModel();
        _ticks = std::move(ticks);
        endResetModel();
    }

    if (step != _step)
    {
        _step = step;
        Q_EMIT stepChanged();
    }
}

}  // namespace curveViewer
