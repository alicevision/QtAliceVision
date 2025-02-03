#pragma once

#include <QQuickItem>
#include <QPainterPath>
#include <QHoverEvent>

#include <cmath>

class MEvent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double x MEMBER m_x CONSTANT)
    Q_PROPERTY(double y MEMBER m_y CONSTANT)
    Q_PROPERTY(Qt::MouseButton button MEMBER m_button CONSTANT)
    Q_PROPERTY(Qt::KeyboardModifiers modifiers MEMBER m_modifiers CONSTANT)

 public:
    explicit MEvent(QMouseEvent* event);
    virtual ~MEvent();

 protected:
    double m_x;
    double m_y;
    Qt::MouseButton m_button;
    Qt::KeyboardModifiers m_modifiers;
};

class EdgeMouse : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(bool containsMouse MEMBER m_containsMouse NOTIFY containsMouseChanged)
    Q_PROPERTY(unsigned int acceptedButtons READ getAcceptedButtons WRITE setAcceptedButtons CONSTANT)
    Q_PROPERTY(double thickness READ getThickness WRITE setThickness NOTIFY thicknessChanged)
    Q_PROPERTY(double curveScale READ getCurveScale WRITE setCurveScale NOTIFY curveScaleChanged)

 public:
    explicit EdgeMouse(QQuickItem* parent = nullptr);
    virtual ~EdgeMouse();

    /// Signals

    Q_SIGNAL void containsMouseChanged();
    Q_SIGNAL void thicknessChanged();
    Q_SIGNAL void curveScaleChanged();
    Q_SIGNAL void pressed(MEvent*);
    Q_SIGNAL void released(MEvent*);

 protected:
    bool contains(const QPointF& point) const override;
    void hoverEnterEvent(QHoverEvent* event) override;
    void hoverLeaveEvent(QHoverEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

 private:
    void setContainsMouse(bool state);

    int getAcceptedButtons() const;
    void setAcceptedButtons(unsigned int buttons);

    double getThickness() const { return m_thickness; }
    void setThickness(double thickness);

    double getCurveScale() const { return m_curveScale; }
    void setCurveScale(double curveScale);

    void updateShape();

 protected:
    double m_curveScale;
    double m_thickness;

    bool m_containsMouse;
    QPainterPath m_path;
};
