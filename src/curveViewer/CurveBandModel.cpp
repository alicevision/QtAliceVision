#include "CurveBandModel.hpp"

#include <QDebug>

#include <algorithm>
#include <cmath>

namespace curveViewer {

namespace {

QRgb toPremultiplied(const QColor& color) { return color.isValid() ? qPremultiply(color.rgba()) : 0; }

/// Linear interpolation between gradient stops evenly spread over [0, 1].
QColor gradientColor(const std::vector<QColor>& stops, double t)
{
    const double f = std::clamp(t, 0.0, 1.0) * static_cast<double>(stops.size() - 1);
    const std::size_t i = std::min(static_cast<std::size_t>(f), stops.size() - 2);
    const float u = static_cast<float>(f - static_cast<double>(i));
    const QColor& a = stops[i];
    const QColor& b = stops[i + 1];
    return QColor::fromRgbF(a.redF() + u * (b.redF() - a.redF()),
                            a.greenF() + u * (b.greenF() - a.greenF()),
                            a.blueF() + u * (b.blueF() - a.blueF()),
                            a.alphaF() + u * (b.alphaF() - a.alphaF()));
}

}  // namespace

int Band::cellAt(double xValue) const
{
    if (x.empty() || !(xValue >= edges.front()) || xValue > edges.back())
    {
        return -1;
    }
    const auto it = std::upper_bound(edges.begin(), edges.end(), xValue);
    return std::min(static_cast<int>(it - edges.begin()) - 1, static_cast<int>(x.size()) - 1);
}

CurveBandModel::CurveBandModel(QObject* parent)
  : QAbstractListModel(parent)
{}

int CurveBandModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(_bands.size());
}

QVariant CurveBandModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidRow(index.row()))
    {
        return {};
    }

    const Band& b = band(index.row());
    switch (role)
    {
        case NameRole:
            return b.name;
        case VisibleRole:
            return b.visible;
        default:
            return {};
    }
}

QHash<int, QByteArray> CurveBandModel::roleNames() const
{
    return {
      {NameRole, "bandName"},
      {VisibleRole, "bandVisible"},
    };
}

int CurveBandModel::addBand(const QString& name, const QList<double>& xs, const QVariantList& colors)
{
    if (xs.size() != colors.size())
    {
        qWarning() << "[CurveBandModel] addBand:" << name << "has" << xs.size() << "x values and" << colors.size() << "colors";
        return -1;
    }

    std::vector<QRgb> rgbs;
    rgbs.reserve(static_cast<std::size_t>(colors.size()));
    for (const QVariant& color : colors)
    {
        rgbs.push_back(toPremultiplied(color.value<QColor>()));
    }
    return insertBand(name, xs, rgbs);
}

int CurveBandModel::addBandFromValues(const QString& name,
                                      const QList<double>& xs,
                                      const QList<double>& values,
                                      const QVariantList& gradient,
                                      double min,
                                      double max)
{
    if (xs.size() != values.size())
    {
        qWarning() << "[CurveBandModel] addBandFromValues:" << name << "has" << xs.size() << "x values and" << values.size() << "values";
        return -1;
    }

    std::vector<QColor> stops;
    for (const QVariant& color : gradient)
    {
        const QColor c = color.value<QColor>();
        if (c.isValid())
        {
            stops.push_back(c);
        }
    }
    if (stops.size() < 2)
    {
        // Viridis
        stops = {QColor(68, 1, 84), QColor(59, 82, 139), QColor(33, 145, 140), QColor(94, 201, 98), QColor(253, 231, 37)};
    }

    if (std::isnan(min) || std::isnan(max))
    {
        double lo = std::numeric_limits<double>::infinity();
        double hi = -std::numeric_limits<double>::infinity();
        for (const double v : values)
        {
            if (std::isfinite(v))
            {
                lo = std::min(lo, v);
                hi = std::max(hi, v);
            }
        }
        if (std::isnan(min))
        {
            min = lo;
        }
        if (std::isnan(max))
        {
            max = hi;
        }
    }

    std::vector<QRgb> rgbs;
    rgbs.reserve(static_cast<std::size_t>(values.size()));
    for (const double v : values)
    {
        if (!std::isfinite(v))
        {
            rgbs.push_back(0);
            continue;
        }
        const double t = max > min ? (v - min) / (max - min) : 0.5;
        rgbs.push_back(toPremultiplied(gradientColor(stops, t)));
    }
    return insertBand(name, xs, rgbs);
}

int CurveBandModel::insertBand(const QString& name, const QList<double>& xs, const std::vector<QRgb>& colors)
{
    // Keep only finite abscissas, sorted by increasing x
    std::vector<std::size_t> order;
    order.reserve(colors.size());
    for (qsizetype i = 0; i < xs.size(); ++i)
    {
        if (std::isfinite(xs[i]))
        {
            order.push_back(static_cast<std::size_t>(i));
        }
    }
    const auto xAt = [&xs](std::size_t i) { return xs[static_cast<qsizetype>(i)]; };
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return xAt(a) < xAt(b); });

    Band b;
    b.id = _nextId++;
    b.name = name;
    b.x.reserve(order.size());
    b.colors.reserve(order.size());
    for (const std::size_t i : order)
    {
        b.x.push_back(xAt(i));
        b.colors.push_back(colors[i]);
    }

    // Cell borders in the middle of consecutive samples, the outer cells mirror their inner half
    const std::size_t n = b.x.size();
    if (n > 0)
    {
        b.edges.resize(n + 1);
        for (std::size_t i = 1; i < n; ++i)
        {
            b.edges[i] = 0.5 * (b.x[i - 1] + b.x[i]);
        }
        b.edges[0] = n > 1 ? b.x[0] - (b.edges[1] - b.x[0]) : b.x[0] - 0.5;
        b.edges[n] = n > 1 ? b.x[n - 1] + (b.x[n - 1] - b.edges[n - 1]) : b.x[0] + 0.5;
    }

    const int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    _bands.push_back(std::move(b));
    endInsertRows();
    Q_EMIT countChanged();
    return row;
}

void CurveBandModel::removeBand(int row)
{
    if (!isValidRow(row))
    {
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    _bands.erase(_bands.begin() + row);
    endRemoveRows();
    Q_EMIT countChanged();
}

void CurveBandModel::clear()
{
    if (_bands.empty())
    {
        return;
    }
    beginResetModel();
    _bands.clear();
    endResetModel();
    Q_EMIT countChanged();
}

void CurveBandModel::setVisible(int row, bool visible)
{
    if (!isValidRow(row) || _bands[static_cast<std::size_t>(row)].visible == visible)
    {
        return;
    }
    _bands[static_cast<std::size_t>(row)].visible = visible;
    const QModelIndex idx = index(row);
    Q_EMIT dataChanged(idx, idx, {VisibleRole});
}

QColor CurveBandModel::colorAt(int row, double x) const
{
    if (!isValidRow(row))
    {
        return {};
    }
    const Band& b = band(row);
    const int cell = b.cellAt(x);
    if (cell < 0 || qAlpha(b.colors[static_cast<std::size_t>(cell)]) == 0)
    {
        return {};
    }
    return QColor::fromRgba(qUnpremultiply(b.colors[static_cast<std::size_t>(cell)]));
}

}  // namespace curveViewer
