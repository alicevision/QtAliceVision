#pragma once

#include <QAbstractListModel>
#include <QString>

#include <vector>

namespace curveViewer {

/**
 * @brief Graduations of a CurveViewer axis.
 *
 * Exposed as a model so that QML repeaters keep their delegates while panning and zooming:
 * the model is only reset when the number of ticks changes, otherwise only dataChanged is emitted.
 */
class CurveAxisTicks : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(double step READ step NOTIFY stepChanged)

  public:
    enum Roles
    {
        PositionRole = Qt::UserRole + 1,
        LabelRole
    };
    Q_ENUM(Roles)

    struct Tick
    {
        double position;  ///< Position in pixels along the axis
        QString label;
    };

    explicit CurveAxisTicks(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Recompute the graduations.
     * @param min Data value at pixel 0 (or at pixel length if inverted).
     * @param max Data value at pixel length (or at pixel 0 if inverted).
     * @param pixelLength Axis length in pixels.
     * @param minSpacing Minimal spacing between two ticks in pixels.
     * @param inverted Whether pixel positions grow when values decrease (vertical axis).
     */
    void update(double min, double max, double pixelLength, double minSpacing, bool inverted);

    double step() const { return _step; }
    const std::vector<Tick>& ticks() const { return _ticks; }

  Q_SIGNALS:
    void stepChanged();

  private:
    std::vector<Tick> _ticks;
    double _step = 1.0;
};

}  // namespace curveViewer
