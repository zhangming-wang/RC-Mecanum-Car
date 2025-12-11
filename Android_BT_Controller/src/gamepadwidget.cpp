#include "gamepadwidget.h"
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTouchEvent>
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

    // 上下两个圆形区域：大小一致，水平居中；与上下边框距离一致
    const qreal circleSize = qMin<qreal>(m_leftFixedH, m_leftFixedW);
    const qreal topW = circleSize;
    const qreal topH = circleSize;
    const qreal bottomW = circleSize;
    const qreal bottomH = circleSize;
    // 计算上下等距：edgeMargin 到顶部圆，上下圆之间的 spacer，到底部圆到底部边框的 edgeMargin
    const qreal totalH = h - 2 * edgeMargin;
    const qreal spacerH = qMax<qreal>(0.0, totalH - topH - bottomH);
    const qreal topY = edgeMargin;
    const qreal bottomY = edgeMargin + topH + spacerH;
    const qreal topX = (w - topW) / 2.0;
    const qreal bottomX = (w - bottomW) / 2.0;
    m_leftArea = QRectF(topX, topY, topW, topH);
    m_rightArea = QRectF(bottomX, bottomY, bottomW, bottomH);

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

    // 下方圆形摇杆绘制（与上方同尺寸，仅上下控制）
    {
        m_radiusLeft = qMax<qreal>(8.0, qMin(m_rightArea.width(), m_rightArea.height()) / 2.0 - spacingInside);
        QPointF centerBottom(m_rightArea.center().x(), m_rightArea.center().y());

        p.setPen(QPen(QColor(180, 180, 180), 3));
        p.setBrush(QColor(60, 60, 60));
        p.drawEllipse(centerBottom, m_radiusLeft, m_radiusLeft);

        p.setPen(QPen(QColor(120, 120, 120), 1));
        // 仅绘制一条竖直中线辅助
        p.drawLine(QPointF(centerBottom.x(), centerBottom.y() - m_radiusLeft), QPointF(centerBottom.x(), centerBottom.y() + m_radiusLeft));

        const qreal knobR = m_radiusLeft * 0.35;
        // 下方圆仅上下移动：x 固定为圆心，y 根据 m_knobZ 映射
        const qreal cy = centerBottom.y() + (m_knobZ)*m_radiusLeft; // 顶部-1时向上
        const qreal cx = centerBottom.x();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 140, 0));
        p.drawEllipse(QPointF(cx, cy), knobR, knobR);
        // 存储方便命中测试
        m_capsule = QRectF(centerBottom.x() - m_radiusLeft, centerBottom.y() - m_radiusLeft, m_radiusLeft * 2, m_radiusLeft * 2);
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
    const qreal y = qBound(r.top(), (qreal)pos.y(), r.bottom());
    // 仅上下映射为 -1..1：上 -1，下 +1
    const qreal t = (y - r.top()) / r.height();
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
        m_leftTouchId = -1;
        m_knobLeft = QPointF(0, 0);
        emit releasedXY();
    }
    if (m_pressedRight) {
        m_pressedRight = false;
        m_rightTouchId = -1;
        m_knobZ = 0.0;
        emit releasedZ();
    }
    update();
}

bool GamepadWidget::event(QEvent *event) {
    if (event->type() == QEvent::TouchBegin || event->type() == QEvent::TouchUpdate || event->type() == QEvent::TouchEnd) {
        auto *te = static_cast<QTouchEvent *>(event);
        const auto points = te->points();
        for (const auto &pt : points) {
            const QPoint pos = pt.position().toPoint();
            const qint64 id = pt.id();
            switch (pt.state()) {
            case QEventPoint::Pressed: {
                if (QLineF(pos, m_centerLeft).length() <= m_radiusLeft * 1.1) {
                    if (m_leftTouchId == -1)
                        m_leftTouchId = id;
                    m_pressedLeft = true;
                    updateCircleFromPos(pos);
                } else if (capsuleRect().contains(pos)) {
                    if (m_rightTouchId == -1)
                        m_rightTouchId = id;
                    m_pressedRight = true;
                    updateCapsuleFromPos(pos);
                }
                break;
            }
            case QEventPoint::Updated: {
                if (id == m_leftTouchId && m_pressedLeft) {
                    updateCircleFromPos(pos);
                } else if (id == m_rightTouchId && m_pressedRight) {
                    updateCapsuleFromPos(pos);
                }
                break;
            }
            case QEventPoint::Released: {
                if (id == m_leftTouchId) {
                    m_leftTouchId = -1;
                    if (m_pressedLeft) {
                        m_pressedLeft = false;
                        m_knobLeft = QPointF(0, 0);
                        emit releasedXY();
                    }
                } else if (id == m_rightTouchId) {
                    m_rightTouchId = -1;
                    if (m_pressedRight) {
                        m_pressedRight = false;
                        m_knobZ = 0.0;
                        emit releasedZ();
                    }
                }
                update();
                break;
            }
            default:
                break;
            }
        }
        event->accept();
        return true;
    }
    return QWidget::event(event);
}
