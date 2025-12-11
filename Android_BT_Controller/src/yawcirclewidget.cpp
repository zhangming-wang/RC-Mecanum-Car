#include "yawcirclewidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

YawCircleWidget::YawCircleWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_AcceptTouchEvents, true);
}

void YawCircleWidget::paintEvent(QPaintEvent *) {
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

    // 旋钮位置（相对中心，仅x）
    QPointF knobCenter = m_center + QPointF(m_knobPos.x(), 0);

    // 旋钮
    const qreal knobR = m_radius * 0.35;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 140, 0));
    p.drawEllipse(knobCenter, knobR, knobR);
}

QPointF YawCircleWidget::clampToCircle(const QPointF &p, qreal radius) const {
    const qreal len = qSqrt(p.x() * p.x() + p.y() * p.y());
    if (len <= radius || len == 0)
        return p;
    const qreal scale = radius / len;
    return QPointF(p.x() * scale, p.y() * scale);
}

void YawCircleWidget::updateKnobFromPos(const QPoint &pos) {
    QPointF rel = QPointF(pos) - m_center; // 相对中心
    rel = clampToCircle(rel, m_radius);
    // 仅保留水平分量，垂直为0，使其为“水平遥感”
    m_knobPos = QPointF(rel.x(), 0);

    // 归一化到 -100..100，右为正，左为负
    const qreal nz = (m_knobPos.x() / m_radius) * 100.0;
    emit moved(qBound(-100, (int)qRound(nz), 100));

    update();
}

void YawCircleWidget::mousePressEvent(QMouseEvent *e) {
    m_pressed = true;
    updateKnobFromPos(e->pos());
}

void YawCircleWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!m_pressed)
        return;
    updateKnobFromPos(e->pos());
}

void YawCircleWidget::mouseReleaseEvent(QMouseEvent *e) {
    Q_UNUSED(e);
    m_pressed = false;
    m_knobPos = QPointF(0, 0);
    emit released();
    update();
}
