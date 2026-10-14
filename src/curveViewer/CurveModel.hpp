#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>

#include <vector>

namespace curveViewer {

/**
 * @brief Axis-aligned bounds of a set of samples, in data space.
 */
struct Bounds
{
    double xMin = 0.0;
    double xMax = 0.0;
    double yMin = 0.0;
    double yMax = 0.0;
    bool valid = false;

    void extend(double x, double y);
    void extend(const Bounds& other);
};

/**
 * @brief Filled area between two bounds sampled at the abscissas of its curve, e.g. min/max or mean +/- sigma.
 */
struct Envelope
{
    std::vector<double> lower;
    std::vector<double> upper;
    /// Opacity applied to the curve color
    float opacity = 0.2f;
};

/**
 * @brief A curve is a polyline through samples sorted by increasing X.
 *
 * It may carry envelopes, drawn as filled areas behind the polyline, to show the uncertainty of its values.
 */
struct Curve
{
    quint64 id = 0;
    QString name;
    /// Group of the curve in the curves list, e.g. its source file, empty if none
    QString group;
    QColor color;
    bool visible = true;
    std::vector<double> x;
    std::vector<double> y;
    /// Drawn back to front, empty for a plain curve
    std::vector<Envelope> envelopes;

    /// Linear interpolation of the curve at x, NaN outside of the curve definition domain.
    double valueAt(double xValue) const;

    /// Linear interpolation of a series sampled at x (y or an envelope bound), NaN outside of the curve definition domain.
    double interpolate(const std::vector<double>& values, double xValue) const;

    /// Bounds of the curve and its envelopes restricted to [x0, x1], including interpolated values at the interval borders.
    Bounds boundsInRange(double x0, double x1) const;
};

/**
 * @brief List model holding the curves displayed by a CurveViewer.
 *
 * Curve samples are immutable once added: to modify a curve, remove it and add it again.
 * Role names are prefixed to avoid clashes with delegate properties (color, visible).
 * Curves of the same group are contiguous, so that views can display them as sections.
 */
class CurveModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

  public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        ColorRole,
        VisibleRole,
        PointCountRole,
        GroupRole,
        HasEnvelopeRole
    };
    Q_ENUM(Roles)

    explicit CurveModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Add a curve.
     * @param name Curve display name.
     * @param xs Sample abscissas (sorted internally if needed).
     * @param ys Sample ordinates, same size as xs.
     * @param color Curve color, an automatic color is picked if invalid.
     * @param group Group of the curve: the curves of a group are kept contiguous, after the existing ones.
     * @return The row of the new curve, -1 if the input is invalid.
     */
    Q_INVOKABLE int addCurve(const QString& name,
                             const QList<double>& xs,
                             const QList<double>& ys,
                             const QColor& color = QColor(),
                             const QString& group = QString());

    /**
     * @brief Add a curve with envelopes showing the uncertainty of its values.
     * @param envelopes List of {lower: [...], upper: [...], opacity: 0.2}, bounds of the same size as xs,
     * drawn back to front (outermost first). A non finite bound collapses the envelope onto the curve.
     * @see addCurve for the other parameters.
     */
    Q_INVOKABLE int addCurveWithEnvelopes(const QString& name,
                                          const QList<double>& xs,
                                          const QList<double>& ys,
                                          const QVariantList& envelopes,
                                          const QColor& color = QColor(),
                                          const QString& group = QString());

    /**
     * @brief Add the mean of a statistic, with a min/max envelope and a mean +/- sigma envelope.
     * @param sigma, min, max Same size as xs, or empty to skip the corresponding envelope.
     * @see addCurve for the other parameters.
     */
    Q_INVOKABLE int addStatisticsCurve(const QString& name,
                                       const QList<double>& xs,
                                       const QList<double>& mean,
                                       const QList<double>& sigma,
                                       const QList<double>& min = QList<double>(),
                                       const QList<double>& max = QList<double>(),
                                       const QColor& color = QColor(),
                                       const QString& group = QString());

    /// Bounds of each envelope of a curve at x, as a list of {lower, upper} (NaN outside of the curve).
    Q_INVOKABLE QVariantList envelopesAt(int row, double x) const;

    Q_INVOKABLE void removeCurve(int row);
    Q_INVOKABLE void clear();

    Q_INVOKABLE void setVisible(int row, bool visible);
    Q_INVOKABLE void setAllVisible(bool visible);

    /**
     * @brief Whether a file has one of the given extensions, e.g. to filter the files dropped on the viewer.
     * @param url Local file url.
     * @param extensions Extensions with their leading dot, compared case insensitively.
     * A file name starting with its only dot (".usda") has no extension.
     */
    Q_INVOKABLE bool isAcceptedFile(const QUrl& url, const QStringList& extensions) const;

    /**
     * @brief Add the camera trajectory of an SfMData file: position X/Y/Z and rotation X/Y/Z per frame.
     *
     * Rotations are accumulated along the frames (sum of the rotation vectors between consecutive frames, world
     * axes, degrees): continuous, and rotating around one axis gives a single growing angle, even past 180 degrees.
     * Views without a pose are skipped. Each camera of a rig gets its own curves.
     * @param url Local SfMData file url.
     * The curves are grouped by the absolute file path, a file already loaded is skipped.
     * @return The number of curves added (0 if the file is already loaded), -1 if the file could not be loaded.
     */
    Q_INVOKABLE int loadFromSfmData(const QUrl& url);

    const Curve& curve(int row) const { return _curves[static_cast<std::size_t>(row)]; }
    bool isValidRow(int row) const { return row >= 0 && row < static_cast<int>(_curves.size()); }

  Q_SIGNALS:
    void countChanged();

  private:
    QColor nextColor();
    /// Envelope bounds as given by the caller, in input order
    struct EnvelopeInput
    {
        QList<double> lower;
        QList<double> upper;
        float opacity;
    };
    int insertCurve(const QString& name,
                    const QList<double>& xs,
                    const QList<double>& ys,
                    const std::vector<EnvelopeInput>& envelopes,
                    const QColor& color,
                    const QString& group);

    std::vector<Curve> _curves;
    quint64 _nextId = 1;
    int _colorIndex = 0;
};

}  // namespace curveViewer
