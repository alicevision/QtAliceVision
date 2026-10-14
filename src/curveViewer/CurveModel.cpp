#include "CurveModel.hpp"

#include <aliceVision/sfmData/SfMData.hpp>
#include <aliceVision/sfmDataIO/sfmDataIO.hpp>

#include <QFileInfo>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numbers>

namespace curveViewer {

void Bounds::extend(double x, double y)
{
    if (!valid)
    {
        xMin = xMax = x;
        yMin = yMax = y;
        valid = true;
        return;
    }
    xMin = std::min(xMin, x);
    xMax = std::max(xMax, x);
    yMin = std::min(yMin, y);
    yMax = std::max(yMax, y);
}

void Bounds::extend(const Bounds& other)
{
    if (!other.valid)
    {
        return;
    }
    extend(other.xMin, other.yMin);
    extend(other.xMax, other.yMax);
}

double Curve::valueAt(double xValue) const { return interpolate(y, xValue); }

double Curve::interpolate(const std::vector<double>& values, double xValue) const
{
    if (x.empty() || xValue < x.front() || xValue > x.back())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const auto it = std::upper_bound(x.begin(), x.end(), xValue);
    if (it == x.end())
    {
        return values.back();
    }
    const std::size_t i1 = static_cast<std::size_t>(it - x.begin());
    if (i1 == 0)
    {
        return values.front();
    }
    const std::size_t i0 = i1 - 1;
    const double dx = x[i1] - x[i0];
    if (dx <= 0.0)
    {
        return values[i1];
    }
    const double t = (xValue - x[i0]) / dx;
    return values[i0] + t * (values[i1] - values[i0]);
}

Bounds Curve::boundsInRange(double x0, double x1) const
{
    Bounds b;
    if (x.empty() || x1 < x.front() || x0 > x.back())
    {
        return b;
    }

    std::vector<const std::vector<double>*> series{&y};
    for (const Envelope& e : envelopes)
    {
        series.insert(series.end(), {&e.lower, &e.upper});
    }

    const auto first = std::lower_bound(x.begin(), x.end(), x0);
    const auto last = std::upper_bound(first, x.end(), x1);
    for (const auto* values : series)
    {
        for (auto it = first; it != last; ++it)
        {
            const std::size_t i = static_cast<std::size_t>(it - x.begin());
            b.extend(x[i], (*values)[i]);
        }

        // Include interpolated values at the interval borders
        for (const double border : {x0, x1})
        {
            const double v = interpolate(*values, border);
            if (std::isfinite(v))
            {
                b.extend(border, v);
            }
        }
    }
    return b;
}

CurveModel::CurveModel(QObject* parent)
  : QAbstractListModel(parent)
{}

int CurveModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(_curves.size());
}

QVariant CurveModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidRow(index.row()))
    {
        return {};
    }

    const Curve& c = curve(index.row());
    switch (role)
    {
        case NameRole:
            return c.name;
        case ColorRole:
            return c.color;
        case VisibleRole:
            return c.visible;
        case PointCountRole:
            return static_cast<int>(c.x.size());
        case GroupRole:
            return c.group;
        case HasEnvelopeRole:
            return !c.envelopes.empty();
        default:
            return {};
    }
}

QHash<int, QByteArray> CurveModel::roleNames() const
{
    return {
      {NameRole, "curveName"},
      {ColorRole, "curveColor"},
      {VisibleRole, "curveVisible"},
      {PointCountRole, "pointCount"},
      {GroupRole, "curveGroup"},
      {HasEnvelopeRole, "curveHasEnvelope"},
    };
}

int CurveModel::addCurve(const QString& name, const QList<double>& xs, const QList<double>& ys, const QColor& color, const QString& group)
{
    return insertCurve(name, xs, ys, {}, color, group);
}

int CurveModel::addCurveWithEnvelopes(const QString& name,
                                      const QList<double>& xs,
                                      const QList<double>& ys,
                                      const QVariantList& envelopes,
                                      const QColor& color,
                                      const QString& group)
{
    std::vector<EnvelopeInput> inputs;
    for (const QVariant& v : envelopes)
    {
        const QVariantMap m = v.toMap();
        inputs.push_back({m.value(QStringLiteral("lower")).value<QList<double>>(),
                          m.value(QStringLiteral("upper")).value<QList<double>>(),
                          m.value(QStringLiteral("opacity"), 0.2).toFloat()});
    }
    return insertCurve(name, xs, ys, inputs, color, group);
}

int CurveModel::addStatisticsCurve(const QString& name,
                                   const QList<double>& xs,
                                   const QList<double>& mean,
                                   const QList<double>& sigma,
                                   const QList<double>& min,
                                   const QList<double>& max,
                                   const QColor& color,
                                   const QString& group)
{
    std::vector<EnvelopeInput> inputs;
    if (!min.isEmpty() || !max.isEmpty())
    {
        inputs.push_back({min.isEmpty() ? mean : min, max.isEmpty() ? mean : max, 0.15f});
    }
    if (!sigma.isEmpty())
    {
        if (sigma.size() != mean.size())
        {
            qWarning() << "[CurveModel] addStatisticsCurve:" << name << "has" << mean.size() << "mean values and" << sigma.size()
                       << "sigma values";
            return -1;
        }
        EnvelopeInput e{mean, mean, 0.3f};
        for (qsizetype i = 0; i < mean.size(); ++i)
        {
            e.lower[i] -= sigma[i];
            e.upper[i] += sigma[i];
        }
        inputs.push_back(std::move(e));
    }
    return insertCurve(name, xs, mean, inputs, color, group);
}

QVariantList CurveModel::envelopesAt(int row, double x) const
{
    QVariantList result;
    if (!isValidRow(row))
    {
        return result;
    }
    const Curve& c = curve(row);
    for (const Envelope& e : c.envelopes)
    {
        result.append(QVariantMap{{QStringLiteral("lower"), c.interpolate(e.lower, x)}, {QStringLiteral("upper"), c.interpolate(e.upper, x)}});
    }
    return result;
}

int CurveModel::insertCurve(const QString& name,
                            const QList<double>& xs,
                            const QList<double>& ys,
                            const std::vector<EnvelopeInput>& envelopes,
                            const QColor& color,
                            const QString& group)
{
    if (xs.size() != ys.size())
    {
        qWarning() << "[CurveModel] addCurve:" << name << "has" << xs.size() << "x values and" << ys.size() << "y values";
        return -1;
    }
    for (const EnvelopeInput& e : envelopes)
    {
        if (e.lower.size() != xs.size() || e.upper.size() != xs.size())
        {
            qWarning() << "[CurveModel] addCurve:" << name << "has" << xs.size() << "x values and an envelope of" << e.lower.size()
                       << "lower and" << e.upper.size() << "upper values";
            return -1;
        }
    }

    // Keep only finite samples, sorted by increasing x
    std::vector<std::size_t> order;
    order.reserve(static_cast<std::size_t>(xs.size()));
    for (qsizetype i = 0; i < xs.size(); ++i)
    {
        if (std::isfinite(xs[i]) && std::isfinite(ys[i]))
        {
            order.push_back(static_cast<std::size_t>(i));
        }
    }
    const auto xAt = [&xs](std::size_t i) { return xs[static_cast<qsizetype>(i)]; };
    if (!std::is_sorted(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return xAt(a) < xAt(b); }))
    {
        std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) { return xAt(a) < xAt(b); });
    }

    Curve c;
    c.id = _nextId++;
    c.name = name;
    c.group = group;
    c.color = color.isValid() ? color : nextColor();
    c.x.reserve(order.size());
    c.y.reserve(order.size());
    for (const std::size_t i : order)
    {
        c.x.push_back(xAt(i));
        c.y.push_back(ys[static_cast<qsizetype>(i)]);
    }
    for (const EnvelopeInput& input : envelopes)
    {
        Envelope e;
        e.opacity = std::clamp(input.opacity, 0.0f, 1.0f);
        e.lower.reserve(order.size());
        e.upper.reserve(order.size());
        for (const std::size_t i : order)
        {
            const auto k = static_cast<qsizetype>(i);
            // A non finite bound collapses the envelope onto the curve
            e.lower.push_back(std::isfinite(input.lower[k]) ? input.lower[k] : ys[k]);
            e.upper.push_back(std::isfinite(input.upper[k]) ? input.upper[k] : ys[k]);
        }
        c.envelopes.push_back(std::move(e));
    }

    // Keep the group contiguous: insert after its last curve, at the end if it is a new group
    const auto last = std::find_if(_curves.rbegin(), _curves.rend(), [&group](const Curve& other) { return other.group == group; });
    const int row = group.isEmpty() || last == _curves.rend() ? rowCount() : static_cast<int>(_curves.rend() - last);
    beginInsertRows(QModelIndex(), row, row);
    _curves.insert(_curves.begin() + row, std::move(c));
    endInsertRows();
    Q_EMIT countChanged();
    return row;
}

void CurveModel::removeCurve(int row)
{
    if (!isValidRow(row))
    {
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    _curves.erase(_curves.begin() + row);
    endRemoveRows();
    Q_EMIT countChanged();
}

void CurveModel::clear()
{
    if (_curves.empty())
    {
        return;
    }
    beginResetModel();
    _curves.clear();
    _colorIndex = 0;
    endResetModel();
    Q_EMIT countChanged();
}

void CurveModel::setVisible(int row, bool visible)
{
    if (!isValidRow(row) || _curves[static_cast<std::size_t>(row)].visible == visible)
    {
        return;
    }
    _curves[static_cast<std::size_t>(row)].visible = visible;
    const QModelIndex idx = index(row);
    Q_EMIT dataChanged(idx, idx, {VisibleRole});
}

void CurveModel::setAllVisible(bool visible)
{
    if (_curves.empty())
    {
        return;
    }
    for (Curve& c : _curves)
    {
        c.visible = visible;
    }
    Q_EMIT dataChanged(index(0), index(rowCount() - 1), {VisibleRole});
}

bool CurveModel::isAcceptedFile(const QUrl& url, const QStringList& extensions) const
{
    const QString name = QFileInfo(url.toLocalFile()).fileName();
    const qsizetype dot = name.lastIndexOf(QLatin1Char('.'));
    return dot > 0 && extensions.contains(name.mid(dot), Qt::CaseInsensitive);
}

int CurveModel::loadFromSfmData(const QUrl& url)
{
    using namespace aliceVision;

    // The curves of a file are grouped by its absolute path: skip a file already loaded
    const QString path = QFileInfo(url.toLocalFile()).absoluteFilePath();
    if (std::any_of(_curves.begin(), _curves.end(), [&path](const Curve& c) { return c.group == path; }))
    {
        return 0;
    }

    sfmData::SfMData sfmData;
    if (!sfmDataIO::load(sfmData, path.toStdString(), sfmDataIO::ESfMData(sfmDataIO::ESfMData::VIEWS | sfmDataIO::ESfMData::EXTRINSICS)))
    {
        qWarning() << "[CurveModel] Failed to load SfMData:" << path;
        return -1;
    }

    struct Sample
    {
        double frame;
        Vec3 center;
        Mat3 rotation;  // camera to world
    };
    std::map<QString, std::vector<Sample>> cameras;

    for (const auto& [viewId, view] : sfmData.getViews())
    {
        if (!sfmData.isPoseDefined(*view))
        {
            continue;
        }

        const geometry::Pose3 pose = sfmData.getPose(*view).getTransform();
        const QString camera = view->isPartOfRig() && !view->isPoseIndependant()
                                 ? QStringLiteral(" rig %1 camera %2").arg(view->getRigId()).arg(view->getSubPoseId())
                                 : QString();
        cameras[camera].push_back({static_cast<double>(view->getFrameId()), pose.center(), pose.rotation().transpose()});
    }

    static const char* names[6] = {"position X", "position Y", "position Z", "rotation X", "rotation Y", "rotation Z"};
    constexpr double toDegrees = 180.0 / std::numbers::pi;
    int added = 0;
    for (auto& [camera, samples] : cameras)
    {
        std::sort(samples.begin(), samples.end(), [](const Sample& a, const Sample& b) { return a.frame < b.frame; });

        QList<double> xs;
        QList<double> ys[6];
        // Accumulated rotation (world axes, degrees): sum of the small rotation vectors between consecutive frames,
        // continuous whatever the motion, and unwrapped past 180 degrees
        Vec3 rotation = Vec3::Zero();
        const Mat3* previous = nullptr;
        for (const Sample& sample : samples)
        {
            if (previous)
            {
                const Eigen::AngleAxisd step(Mat3(sample.rotation * previous->transpose()));
                rotation += step.axis() * step.angle() * toDegrees;
            }
            previous = &sample.rotation;

            xs.push_back(sample.frame);
            for (int k = 0; k < 3; ++k)
            {
                ys[k].push_back(sample.center[k]);
                ys[3 + k].push_back(rotation[k]);
            }
        }

        for (int k = 0; k < 6; ++k)
        {
            if (addCurve(QStringLiteral("%1 %2").arg(camera, QLatin1String(names[k])).trimmed(), xs, ys[k], QColor(), path) >= 0)
            {
                ++added;
            }
        }
    }
    return added;
}

QColor CurveModel::nextColor()
{
    // Golden ratio hue sequence: consecutive curves get well separated hues
    constexpr double goldenRatioConjugate = 0.618033988749895;
    const double hue = std::fmod(0.07 + goldenRatioConjugate * _colorIndex, 1.0);
    // Alternate saturation/value slightly to separate curves with close hues
    const double saturation = (_colorIndex / 7) % 2 == 0 ? 0.70 : 0.50;
    const double value = (_colorIndex / 3) % 2 == 0 ? 0.95 : 0.80;
    ++_colorIndex;
    return QColor::fromHsvF(static_cast<float>(hue), static_cast<float>(saturation), static_cast<float>(value));
}

}  // namespace curveViewer
