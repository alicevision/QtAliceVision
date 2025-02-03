#include "EdgeMouse.hpp"

#include <QVector2D>

EdgeMouse::EdgeMouse(QQuickItem* parent)
  : QQuickItem(parent),
    m_curveScale(0.7),
    m_thickness(2.f),
    m_containsMouse(false),
    m_path()
{
    setAcceptHoverEvents(true);
}

EdgeMouse::~EdgeMouse()
{
}

void EdgeMouse::hoverEnterEvent(QHoverEvent* event)
{
    Q_UNUSED(event);
    setContainsMouse(true);
}

void EdgeMouse::hoverLeaveEvent(QHoverEvent* event)
{
    Q_UNUSED(event);
    setContainsMouse(false);
}

void EdgeMouse::setContainsMouse(bool state)
{
    if (m_containsMouse == state)
    {
        return;
    }

    m_containsMouse = state;
    Q_EMIT containsMouseChanged();
}

bool EdgeMouse::contains(const QPointF& point) const
{
    return m_path.contains(point);
}

int EdgeMouse::getAcceptedButtons() const
{
    return acceptedMouseButtons();
}

void EdgeMouse::setAcceptedButtons(unsigned int buttons)
{
    setAcceptedMouseButtons(static_cast<Qt::MouseButtons>(buttons));
}

void EdgeMouse::setThickness(double thickness)
{
    m_thickness = thickness;
    Q_EMIT thicknessChanged();

    updateShape();
}

void EdgeMouse::setCurveScale(double curveScale)
{
    m_curveScale = curveScale;
    Q_EMIT curveScaleChanged();

    updateShape();
}

void EdgeMouse::updateShape()
{
    QPointF p1(0, 0);
    QPointF p2(width(), height());
    QPointF ctrlPoint(std::abs(width() * m_curveScale), 0);

    QPainterPath path(p1);
    path.cubicTo(p1 + ctrlPoint, p2 - ctrlPoint, p2);

    // Compute offset on x and y axis
    double halfThickness = m_thickness / 2.f;
    QVector2D v = QVector2D(p2 - p1).normalized();

    QPointF offset(halfThickness * (-(v.y())), halfThickness * v.x());

    m_path = QPainterPath(path.toReversed());
    m_path.translate(-(offset));
    path.translate(offset);

    m_path.connectPath(path);
}

void EdgeMouse::mousePressEvent(QMouseEvent* event)
{
    if (! (acceptedMouseButtons() & event->button()))
    {
        event->setAccepted(false);
        return;
    }
    MEvent evt(event);
    Q_EMIT pressed(&evt);
}

void EdgeMouse::mouseReleaseEvent(QMouseEvent* event)
{
    MEvent evt(event);
    Q_EMIT released(&evt);
}

void EdgeMouse::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    // Invoke update shape to update the path with every geometryChange on the parent QQuickItem
    updateShape();
}

MEvent::MEvent(QMouseEvent* event)
  : m_x(event->position().x()),
    m_y(event->position().y()),
    m_button(event->button()),
    m_modifiers(event->modifiers())
{
}

MEvent::~MEvent()
{
}
