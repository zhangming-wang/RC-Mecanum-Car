#include "dualstickwidget.h"
#include <QMouseEvent>
#include <QPainter>

DualStickWidget::DualStickWidget(QWidget *parent) : QWidget(parent) {}

QRectF DualStickWidget::leftTrack() const {
    const qreal w = width();
    const qreal h = height();
    const qreal margin = 16.0;
    const qreal trackW = (w - 3 * margin) / 2.0;
    const qreal trackH = 16.0;
    return QRectF(margin, h / 2.0 - trackH / 2.0, trackW, trackH);
}

QRectF DualStickWidget::rightTrack() const {
    const qreal w = width();
    const qreal h = height();
    const qreal margin = 16.0;
    const qreal trackW = (w - 3 * margin) / 2.0;
    const qreal trackH = 16.0;
    return QRectF(2 * margin + trackW, h / 2.0 - trackH / 2.0, trackW, trackH);
}

void DualStickWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    auto lt = leftTrack();
    auto rt = rightTrack();

    // Draw tracks
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(220, 220, 220));
    p.drawRoundedRect(lt, 8, 8);
    p.drawRoundedRect(rt, 8, 8);

    // center marks
    p.setBrush(QColor(180, 180, 180));
    p.drawRect(QRectF(lt.center().x() - 1, lt.top(), 2, lt.height()));
    p.drawRect(QRectF(rt.center().x() - 1, rt.top(), 2, rt.height()));

    // knobs
    const qreal leftCx = lt.center().x() + m_leftX * (lt.width() / 2.0 - m_radius);
    const qreal rightCx = rt.center().x() + m_rightX * (rt.width() / 2.0 - m_radius);
    const QPointF leftCenter(leftCx, lt.center().y());
    const QPointF rightCenter(rightCx, rt.center().y());

    p.setBrush(QColor(100, 149, 237));
    p.setPen(QPen(QColor(80, 80, 80), 1));
    p.drawEllipse(leftCenter, m_radius, m_radius);
    p.drawEllipse(rightCenter, m_radius, m_radius);

    // labels
    p.setPen(QPen(QColor(90, 90, 90)));
    p.drawText(lt.adjusted(0, -26, 0, -26), Qt::AlignHCenter, tr("X"));
    p.drawText(rt.adjusted(0, -26, 0, -26), Qt::AlignHCenter, tr("Y"));
}

void DualStickWidget::mousePressEvent(QMouseEvent *e) {
    m_pressed = true;
    auto lt = leftTrack();
    auto rt = rightTrack();
    if (lt.contains(e->pos()))
        m_activeTrack = 1;
    else if (rt.contains(e->pos()))
        m_activeTrack = 2;
    else
        m_activeTrack = 0;
    updateKnobFromPos(e->pos());
}

void DualStickWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!m_pressed)
        return;
    updateKnobFromPos(e->pos());
}

void DualStickWidget::mouseReleaseEvent(QMouseEvent *) {
    m_pressed = false;
    m_activeTrack = 0;
    // return both to center
    m_leftX = 0.0;
    m_rightX = 0.0;
    update();
    emit released();
}

void DualStickWidget::updateKnobFromPos(const QPoint &pos) {
    auto lt = leftTrack();
    auto rt = rightTrack();
    const auto track = (m_activeTrack == 1 ? lt : (m_activeTrack == 2 ? rt : QRectF()));
    if (track.isNull())
        return;
    const qreal cx = track.center().x();
    const qreal half = (track.width() / 2.0 - m_radius);
    qreal dx = (pos.x() - cx);
    if (dx > half)
        dx = half;
    if (dx < -half)
        dx = -half;
    qreal norm = dx / half; // -1..1
    if (m_activeTrack == 1)
        m_leftX = norm;
    else if (m_activeTrack == 2)
        m_rightX = norm;

    const int x = static_cast<int>(m_leftX * 100.0);
    const int y = static_cast<int>(m_rightX * 100.0);
    emit moved(x, y);
    update();
}
