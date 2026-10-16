#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QList>
#include <QRgb>
#include <QString>
#include <QVariantList>

#include <limits>
#include <vector>

namespace curveViewer {

/**
 * @brief A band is a row of colored cells along X, one cell per sample.
 *
 * Each cell is centered on its sample and spans up to the middle of its neighbors.
 */
struct Band
{
    quint64 id = 0;
    QString name;
    bool visible = true;
    /// Cell centers, sorted by increasing X
    std::vector<double> x;
    /// Cell borders, x.size() + 1 values: cell i spans [edges[i], edges[i + 1]]
    std::vector<double> edges;
    /// Premultiplied cell colors, a fully transparent cell is a gap
    std::vector<QRgb> colors;

    /// Index of the cell containing xValue, -1 if outside of the band.
    int cellAt(double xValue) const;
};

/**
 * @brief List model holding the color bands displayed on top of a CurveViewer.
 *
 * Band samples are immutable once added: to modify a band, remove it and add it again.
 * Role names are prefixed to avoid clashes with delegate properties (visible).
 */
class CurveBandModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

  public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1,
        VisibleRole
    };
    Q_ENUM(Roles)

    explicit CurveBandModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Add a band with one color per sample.
     * @param name Band display name.
     * @param xs Sample abscissas (sorted internally if needed).
     * @param colors Cell colors (QML colors or color strings), same size as xs. An invalid color is a gap.
     * @return The row of the new band, -1 if the input is invalid.
     */
    Q_INVOKABLE int addBand(const QString& name, const QList<double>& xs, const QVariantList& colors);

    /**
     * @brief Add a band whose colors map a series of values onto a gradient.
     * @param name Band display name.
     * @param xs Sample abscissas (sorted internally if needed).
     * @param values Sample values, same size as xs. A non finite value is a gap.
     * @param gradient Colors evenly spread from min to max, a viridis gradient if empty.
     * @param min, max Values mapped to the gradient ends, the extent of the finite values if NaN.
     * @return The row of the new band, -1 if the input is invalid.
     */
    Q_INVOKABLE int addBandFromValues(const QString& name,
                                      const QList<double>& xs,
                                      const QList<double>& values,
                                      const QVariantList& gradient = QVariantList(),
                                      double min = std::numeric_limits<double>::quiet_NaN(),
                                      double max = std::numeric_limits<double>::quiet_NaN());

    Q_INVOKABLE void removeBand(int row);
    Q_INVOKABLE void clear();

    Q_INVOKABLE void setVisible(int row, bool visible);

    /// Color of a band at x, invalid if outside of the band or in a gap.
    Q_INVOKABLE QColor colorAt(int row, double x) const;

    const Band& band(int row) const { return _bands[static_cast<std::size_t>(row)]; }
    bool isValidRow(int row) const { return row >= 0 && row < static_cast<int>(_bands.size()); }

  Q_SIGNALS:
    void countChanged();

  private:
    int insertBand(const QString& name, const QList<double>& xs, const std::vector<QRgb>& colors);

    std::vector<Band> _bands;
    quint64 _nextId = 1;
};

}  // namespace curveViewer
