#include "gamepadwidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

GamepadWidget::GamepadWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_AcceptTouchEvents, true);
}

void GamepadWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 背景
    p.fillRect(rect(), QColor(20, 20, 20));

    const int w = width();
    const int h = height();
    const qreal edgeMargin = 50.0;    // 边缘留白
    const qreal spacingInside = 20.0; // 区域内部留白

    // 左右区域分配：两侧控件宽高固定，spacer为剩余水平空间（expanding），垂直居中
    const qreal totalW = w - 2 * edgeMargin;
    const qreal leftW = m_leftFixedW;
    const qreal rightW = m_rightFixedW;
    const qreal spacerW = qMax<qreal>(0.0, totalW - leftW - rightW);
    const qreal leftX = edgeMargin;
    const qreal rightX = edgeMargin + leftW + spacerW;
    const qreal leftH = m_leftFixedH;
    const qreal rightH = m_rightFixedH;
    const qreal leftY = (h - leftH) / 2.0;
    const qreal rightY = (h - rightH) / 2.0;
    m_leftArea = QRectF(leftX, leftY, leftW, leftH);
    m_rightArea = QRectF(rightX, rightY, rightW, rightH);

    // 左侧圆形摇杆绘制
    {
        // 确保左圆到左边界与右胶囊到右边界的距离一致：使用相同的内部留白spacingInside
        const qreal leftMaxRadiusX = m_leftArea.width() / 2.0 - spacingInside;
        const qreal leftMaxRadiusY = m_leftArea.height() / 2.0 - spacingInside;
        m_radiusLeft = qMax<qreal>(8.0, qMin(leftMaxRadiusX, leftMaxRadiusY));
        m_centerLeft = QPointF(m_leftArea.center().x(), m_leftArea.center().y());

        p.setPen(QPen(QColor(180, 180, 180), 3));
        p.setBrush(QColor(60, 60, 60));
        p.drawEllipse(m_centerLeft, m_radiusLeft, m_radiusLeft);

        p.setPen(QPen(QColor(120, 120, 120), 1));
        p.drawLine(QPointF(m_centerLeft.x() - m_radiusLeft, m_centerLeft.y()), QPointF(m_centerLeft.x() + m_radiusLeft, m_centerLeft.y()));
        p.drawLine(QPointF(m_centerLeft.x(), m_centerLeft.y() - m_radiusLeft), QPointF(m_centerLeft.x(), m_centerLeft.y() + m_radiusLeft));

        const qreal knobR = m_radiusLeft * 0.35; // 中心圆尺寸（左右一致）
        QPointF knobCenter = m_centerLeft + m_knobLeft;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 170, 255));
        p.drawEllipse(knobCenter, knobR, knobR);
    }

    // 右侧胶囊摇杆绘制（水平，仅左右）
    {
        const qreal rw = m_rightArea.width();
        const qreal rh = m_rightArea.height();
        // 胶囊高度为左圆直径的一半（即左半径数值），并使用与左侧相同的内部留白spacingInside
        const qreal capsuleH = qMin(rh, m_radiusLeft);
        const qreal capsuleW = rw - 2 * spacingInside; // 左右留白一致
        m_capsuleRadius = capsuleH / 2.0;
        const qreal x = m_rightArea.left() + spacingInside;
        const qreal y = m_rightArea.center().y() - capsuleH / 2.0;
        m_capsule = QRectF(x, y, capsuleW, capsuleH);

        // 胶囊外观
        QPainterPath path;
        QRectF r = m_capsule;
        path.addRoundedRect(r, m_capsuleRadius, m_capsuleRadius);
        p.setPen(QPen(QColor(180, 180, 180), 3));
        p.setBrush(QColor(60, 60, 60));
        p.drawPath(path);

        // 中线
        p.setPen(QPen(QColor(120, 120, 120), 1));
        p.drawLine(QPointF(r.left(), r.center().y()), QPointF(r.right(), r.center().y()));

        // 旋钮位置：根据 m_knobZ (-1..1) 映射到胶囊内
        // 中心圆尺寸与左侧一致
        const qreal knobR = m_radiusLeft * 0.35;
        const qreal cx = r.left() + (m_knobZ + 1.0) * 0.5 * r.width();
        const qreal cy = r.center().y();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 140, 0));
        p.drawEllipse(QPointF(cx, cy), knobR, knobR);
    }
}

QPointF GamepadWidget::clampToCircle(const QPointF &p, qreal radius) const {
    const qreal len = qSqrt(p.x() * p.x() + p.y() * p.y());
    if (len <= radius || len == 0)
        return p;
    const qreal scale = radius / len;
    return QPointF(p.x() * scale, p.y() * scale);
}

void GamepadWidget::updateCircleFromPos(const QPoint &pos) {
    QPointF rel = QPointF(pos) - m_centerLeft; // 相对中心
    rel = clampToCircle(rel, m_radiusLeft);
    m_knobLeft = rel;

    const qreal nx = (m_knobLeft.x() / m_radiusLeft);
    const qreal ny = (-m_knobLeft.y() / m_radiusLeft);
    emit movedXY(qBound(-1.0, nx, 1.0), qBound(-1.0, ny, 1.0));
    update();
}

QRectF GamepadWidget::capsuleRect() const {
    return m_capsule;
}

void GamepadWidget::updateCapsuleFromPos(const QPoint &pos) {
    const QRectF r = capsuleRect();
    const qreal x = qBound(r.left(), (qreal)pos.x(), r.right());
    // 将 x 映射为 -1..1
    const qreal t = (x - r.left()) / r.width();
    m_knobZ = t * 2.0 - 1.0;

    const qreal z = qBound(-1.0, m_knobZ, 1.0);
    emit movedZ(z);
    update();
}

void GamepadWidget::mousePressEvent(QMouseEvent *e) {
    const QPoint pt = e->pos();
    if (QLineF(pt, m_centerLeft).length() <= m_radiusLeft * 1.1) {
        m_pressedLeft = true;
        updateCircleFromPos(pt);
        return;
    }
    if (capsuleRect().contains(pt)) {
        m_pressedRight = true;
        updateCapsuleFromPos(pt);
        return;
    }
}

void GamepadWidget::mouseMoveEvent(QMouseEvent *e) {
    const QPoint pt = e->pos();
    if (m_pressedLeft) {
        updateCircleFromPos(pt);
    } else if (m_pressedRight) {
        updateCapsuleFromPos(pt);
    }
}

void GamepadWidget::mouseReleaseEvent(QMouseEvent *e) {
    Q_UNUSED(e);
    if (m_pressedLeft) {
        m_pressedLeft = false;
        m_knobLeft = QPointF(0, 0);
        emit releasedXY();
    }
    if (m_pressedRight) {
        m_pressedRight = false;
        m_knobZ = 0.0;
        emit releasedZ();
    }
    update();
}
