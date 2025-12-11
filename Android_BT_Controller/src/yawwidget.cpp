#include "yawwidget.h"
#include <QMouseEvent>
#include <QPainter>

YawWidget::YawWidget(QWidget *parent) : QWidget(parent) {}

void YawWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal w = width();
    const qreal h = height();
    const qreal margin = 12.0;
    m_trackRect = QRectF(margin, h / 2.0 - 8.0, w - 2 * margin, 16.0);

    // 轨道
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(220, 220, 220));
    p.drawRoundedRect(m_trackRect, 8, 8);

    // 刻度中心线
    p.setBrush(QColor(180, 180, 180));
    p.drawRect(QRectF(w / 2.0 - 1.0, m_trackRect.top(), 2.0, m_trackRect.height()));

    // 旋钮位置
    const qreal cx = m_trackRect.center().x();
    const qreal knobCenterX = cx + m_knobX * (m_trackRect.width() / 2.0 - m_radius);
    const QPointF knobCenter(knobCenterX, m_trackRect.center().y());

    // 旋钮
    p.setBrush(QColor(100, 149, 237));
    p.setPen(QPen(QColor(80, 80, 80), 1));
    p.drawEllipse(knobCenter, m_radius, m_radius);
}

void YawWidget::mousePressEvent(QMouseEvent *e) {
    m_pressed = true;
    updateKnobFromPos(e->pos());
}

void YawWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!m_pressed)
        return;
    updateKnobFromPos(e->pos());
}

void YawWidget::mouseReleaseEvent(QMouseEvent *) {
    m_pressed = false;
    m_knobX = 0.0;
    update();
    emit released();
}

void YawWidget::updateKnobFromPos(const QPoint &pos) {
    // 将鼠标X映射到 -1..1
    const qreal cx = m_trackRect.center().x();
    const qreal half = (m_trackRect.width() / 2.0 - m_radius);
    qreal dx = (pos.x() - cx);
    if (dx > half)
        dx = half;
    if (dx < -half)
        dx = -half;
    m_knobX = dx / half;

    // 发射 -100..100 的z值
    const int z = static_cast<int>(m_knobX * 100.0);
    emit moved(z);
    update();
}
