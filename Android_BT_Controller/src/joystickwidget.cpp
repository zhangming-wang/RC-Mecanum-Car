#include "joystickwidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

JoystickWidget::JoystickWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_AcceptTouchEvents, true);
}

void JoystickWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();
    m_center = QPointF(w / 2.0, h / 2.0);
    m_radius = qMin(w, h) * 0.42; // 留边距

    // 背景
    p.fillRect(rect(), QColor(20, 20, 20));

    // 外圈
    p.setPen(QPen(QColor(180, 180, 180), 3));
    p.setBrush(QColor(60, 60, 60));
    p.drawEllipse(m_center, m_radius, m_radius);

    // 十字线
    p.setPen(QPen(QColor(120, 120, 120), 1));
    p.drawLine(QPointF(m_center.x() - m_radius, m_center.y()), QPointF(m_center.x() + m_radius, m_center.y()));
    p.drawLine(QPointF(m_center.x(), m_center.y() - m_radius), QPointF(m_center.x(), m_center.y() + m_radius));

    // 旋钮位置（相对中心）
    QPointF knobCenter = m_center + m_knobPos;

    // 旋钮
    const qreal knobR = m_radius * 0.35;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 170, 255));
    p.drawEllipse(knobCenter, knobR, knobR);
}

QPointF JoystickWidget::clampToCircle(const QPointF &p, qreal radius) const {
    const qreal len = qSqrt(p.x() * p.x() + p.y() * p.y());
    if (len <= radius || len == 0)
        return p;
    const qreal scale = radius / len;
    return QPointF(p.x() * scale, p.y() * scale);
}

void JoystickWidget::updateKnobFromPos(const QPoint &pos) {
    QPointF rel = QPointF(pos) - m_center; // 相对中心
    rel = clampToCircle(rel, m_radius);
    m_knobPos = rel;

    // 归一化到 -100..100，注意 Y 轴向上为正
    const qreal nx = (m_knobPos.x() / m_radius) * 100.0;
    const qreal ny = (-m_knobPos.y() / m_radius) * 100.0;
    emit moved(qBound(-100, (int)qRound(nx), 100), qBound(-100, (int)qRound(ny), 100));

    update();
}

void JoystickWidget::mousePressEvent(QMouseEvent *e) {
    m_pressed = true;
    updateKnobFromPos(e->pos());
}

void JoystickWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!m_pressed)
        return;
    updateKnobFromPos(e->pos());
}

void JoystickWidget::mouseReleaseEvent(QMouseEvent *e) {
    Q_UNUSED(e);
    m_pressed = false;
    m_knobPos = QPointF(0, 0);
    emit released();
    update();
}
