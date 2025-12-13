#include "gamepadwidget.h"

// 集中化UI参数，便于统一调整与提高可读性
namespace {
    // 计时/动画
    constexpr int kBlinkIntervalMs = 500;
    // 触控双击（双击）判定间隔
    constexpr int kDoubleTapIntervalMs = 350;

    // 摇杆圆与按钮
    constexpr qreal kMinCircleRadius = 8.0; // 圆形区域最小半径
    constexpr qreal kKnobRatio = 0.35;      // 小圆按钮相对于半径的比例

    // 状态指示器
    constexpr int kIndicatorInnerPad = 4;     // 指示器内圈padding
    constexpr int kIndicatorReserveExtra = 4; // 滑块区域为指示器额外预留的像素

    // 滑块几何
    constexpr int kSliderMinWidth = 240;  // 滑块最小宽度
    constexpr int kSliderSidePadding = 8; // 滑块左右额外留白（相对可用空间）
    constexpr int kSliderHeight = 48;     // 滑块高度
    constexpr int kSliderLeftBias = 12;   // 滑块整体向左偏移（视觉微调）

    // 滑块样式（样式表用）
    constexpr int kGrooveHeight = 14;  // 轨道高度
    constexpr int kHandleWidth = 40;   // 手柄宽度
    constexpr int kHandleMarginY = 18; // 手柄上下外边距（负值扩大触区）
    constexpr int kHandleRadius = 20;  // 手柄圆角
} // namespace

GamepadWidget::GamepadWidget(QWidget *parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_AcceptTouchEvents, true);

    m_bt = new BluetoothClient(this);
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    // 让滑块自身接受触控并具备焦点，支持多指同时操作
    m_speedSlider->setAttribute(Qt::WA_AcceptTouchEvents, true);
    m_speedSlider->setFocusPolicy(Qt::StrongFocus);
    m_speedSlider->setMouseTracking(true);

    // 中间速度滑块（水平，0-100%）
    m_speedSlider->setRange(0, 100);
    m_speedSlider->setValue(0);
    // 风格统一：深色轨道 + 高亮橙色手柄，圆角（从集中常量生成，便于统一调整）
    m_speedSlider->setStyleSheet(QString(
                                     "QSlider::groove:horizontal {\n"
                                     "  height: %1px;\n"
                                     "  background: #3c3c3c;\n"
                                     "  border-radius: 3px;\n"
                                     "}\n"
                                     "QSlider::sub-page:horizontal {\n"
                                     "  background: #0099ff;\n"
                                     "  border-radius: 3px;\n"
                                     "}\n"
                                     "QSlider::add-page:horizontal {\n"
                                     "  background: #3c3c3c;\n"
                                     "  border-radius: 3px;\n"
                                     "}\n"
                                     "QSlider::handle:horizontal {\n"
                                     "  background: #ff8c00;\n"
                                     "  border: 2px solid #c56f00;\n"
                                     "  width: %2px;\n"
                                     "  margin: -%3px 0;\n"
                                     "  border-radius: %4px;\n"
                                     "}\n")
                                     .arg(kGrooveHeight)
                                     .arg(kHandleWidth)
                                     .arg(kHandleMarginY)
                                     .arg(kHandleRadius));

    connect(m_speedSlider, &QSlider::sliderReleased, this, [this]() {
        emit speedChanged(static_cast<double>(m_speedSlider->value()) / m_speedSlider->maximum());
    });

    connect(m_bt, &BluetoothClient::connected, this, [this]() {
        setConnected(true);
        m_speedSlider->setValue(m_speedSlider->maximum()); // 连接时重置速度
        emit speedChanged(1.0);
    });
    connect(m_bt, &BluetoothClient::disconnected, this, [this]() {
        setConnected(false);
        m_speedSlider->setValue(0); // 断开时速度归零
    });
    connect(m_bt, &BluetoothClient::error, this, [](const QString &msg) {
        qWarning() << "Bluetooth error:" << msg;
    });
    connect(m_bt, &BluetoothClient::messageReceived, this, [](const QByteArray &data) {
        qDebug() << "Received data:" << data;
    });

    connect(this, &GamepadWidget::movedXY, this, [this](double x, double y) { onMoveChanged(); });
    connect(this, &GamepadWidget::releasedXY, this, [this]() { onMoveChanged(); });
    connect(this, &GamepadWidget::movedZ, this, [this](double z) { onMoveChanged(); });
    connect(this, &GamepadWidget::releasedZ, this, [this]() { onMoveChanged(); });
    connect(this, &GamepadWidget::speedChanged, this, &GamepadWidget::onSpeedChanged);

    // 断开时闪烁
    m_blinkTimer.setInterval(kBlinkIntervalMs);
    connect(&m_blinkTimer, &QTimer::timeout, this, [this]() {
        if (!m_connected) {
            m_blinkOn = !m_blinkOn;
            update();
        }
    });
    m_blinkTimer.start();
}

void GamepadWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 背景
    p.fillRect(rect(), QColor(20, 20, 20));

    const int w = width();
    const int h = height();
    const qreal edgeMargin = m_edgeMargin;       // 边缘留白
    const qreal spacingInside = m_spacingInside; // 区域内部留白

    // 上下两个圆形区域：大小一致，水平居中；与上下边框距离一致
    const qreal circleSize = qMin<qreal>(m_topFixedH, m_topFixedW);
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
    m_topArea = QRectF(topX, topY, topW, topH);
    m_bottomArea = QRectF(bottomX, bottomY, bottomW, bottomH);

    // 上方圆形摇杆绘制（XY）
    {
        // 使用相同的内部留白 spacingInside
        const qreal maxRadiusX = m_topArea.width() / 2.0 - spacingInside;
        const qreal maxRadiusY = m_topArea.height() / 2.0 - spacingInside;
        m_topRadius = qMax<qreal>(kMinCircleRadius, qMin(maxRadiusX, maxRadiusY));
        m_topCenter = QPointF(m_topArea.center().x(), m_topArea.center().y());

        p.setPen(QPen(QColor(180, 180, 180), 3));
        p.setBrush(QColor(60, 60, 60));
        p.drawEllipse(m_topCenter, m_topRadius, m_topRadius);

        p.setPen(QPen(QColor(120, 120, 120), 1));
        p.drawLine(QPointF(m_topCenter.x() - m_topRadius, m_topCenter.y()), QPointF(m_topCenter.x() + m_topRadius, m_topCenter.y()));
        p.drawLine(QPointF(m_topCenter.x(), m_topCenter.y() - m_topRadius), QPointF(m_topCenter.x(), m_topCenter.y() + m_topRadius));

        const qreal knobR = m_topRadius * kKnobRatio; // 中心圆尺寸
        QPointF knobCenter = m_topCenter + m_knobXY;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 170, 255));
        p.drawEllipse(knobCenter, knobR, knobR);
    }

    // 中间速度滑块几何
    {
        const qreal edgeMargin = m_edgeMargin;
        const qreal circleSize = qMin<qreal>(m_topFixedH, m_topFixedW);
        const qreal topH = circleSize;
        const qreal bottomH = circleSize;
        const qreal totalH = h - 2 * edgeMargin;
        const qreal spacerH = qMax<qreal>(0.0, totalH - topH - bottomH);
        const qreal spacerX = edgeMargin;
        // 预留右侧状态指示器空间，避免滑块遮挡
        const int indicatorReserve = m_indicatorSize + kIndicatorReserveExtra; // 更贴右侧，仅保留极小空隙
        const qreal spacerW = w - 2 * edgeMargin - indicatorReserve;
        const qreal spacerY = edgeMargin + topH;
        const int sliderW = qMax<int>(kSliderMinWidth, (int)spacerW - kSliderSidePadding); // 明显加长，尽量利用中间空间
        const int sliderH = kSliderHeight;                                                 // 显著增高，更易触控
                                                                                           // 轻微左移，仍保持近似居中
        const int sliderX = (int)(spacerX + (spacerW - sliderW) / 2) - kSliderLeftBias;
        const int sliderY = (int)(spacerY + (spacerH - sliderH) / 2);
        if (m_speedSlider && sliderW > 0 && spacerH > 0)
            m_speedSlider->setGeometry(sliderX, sliderY, sliderW, sliderH);
    }

    // 右侧连接状态指示器：圆形灯（恢复之前样式）
    {
        const QRectF r = indicatorRect();
        const int x = (int)r.x();
        const int y = (int)r.y();
        const int indicatorSize = (int)r.width();

        // 外圈（灰色边框）
        p.setPen(QPen(QColor(120, 120, 120), 2));
        p.setBrush(QColor(40, 40, 40));
        p.drawEllipse(QRectF(x, y, indicatorSize, indicatorSize));

        // 内核：未连接时红色闪烁，已连接常亮绿色
        QColor greenOn = m_indicatorOnColor; // 常亮绿色
        QColor redOn(220, 40, 40);           // 红色亮态
        QColor redOff = m_indicatorOffColor; // 灭态（暗灰）
        QColor fill = m_connected ? greenOn : (m_blinkOn ? redOn : redOff);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        const qreal innerPad = kIndicatorInnerPad;
        p.drawEllipse(QRectF(x + innerPad, y + innerPad, indicatorSize - innerPad * 2, indicatorSize - innerPad * 2));
    }

    // 下方圆形摇杆绘制（与上方同尺寸，仅垂直方向控制Z）
    {
        qreal bottomRadius = qMax<qreal>(8.0, qMin(m_bottomArea.width(), m_bottomArea.height()) / 2.0 - spacingInside);
        QPointF centerBottom(m_bottomArea.center().x(), m_bottomArea.center().y());

        p.setPen(QPen(QColor(180, 180, 180), 3));
        p.setBrush(QColor(60, 60, 60));
        p.drawEllipse(centerBottom, bottomRadius, bottomRadius);

        p.setPen(QPen(QColor(120, 120, 120), 1));
        // 仅绘制一条竖直中线辅助
        p.drawLine(QPointF(centerBottom.x(), centerBottom.y() - bottomRadius), QPointF(centerBottom.x(), centerBottom.y() + bottomRadius));

        const qreal knobR = bottomRadius * kKnobRatio;
        // 下方圆仅上下移动：x 固定为圆心，y 根据 m_knobZ 映射
        const qreal cy = centerBottom.y() + (m_knobZ)*bottomRadius; // 顶部-1时向上
        const qreal cx = centerBottom.x();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 170, 255));
        p.drawEllipse(QPointF(cx, cy), knobR, knobR);
        // 存储方便命中测试（下方圆形区域）
        m_bottomCircle = QRectF(centerBottom.x() - bottomRadius, centerBottom.y() - bottomRadius, bottomRadius * 2, bottomRadius * 2);
    }
}

QRectF GamepadWidget::indicatorRect() const {
    const int w = width();
    const int h = height();
    const int indicatorSize = m_indicatorSize;
    const int x = w - indicatorSize - qMax(0, (int)m_edgeMargin / 2);
    const int y = (h - indicatorSize) / 2;
    return QRectF(x, y, indicatorSize, indicatorSize);
}

bool GamepadWidget::pointInIndicator(const QPoint &pt) const {
    const QRectF r = indicatorRect();
    if (!r.contains(pt))
        return false;
    const QPointF c = r.center();
    const qreal dx = pt.x() - c.x();
    const qreal dy = pt.y() - c.y();
    const qreal rr = (r.width() * 0.5);
    return (dx * dx + dy * dy) <= rr * rr;
}

QPointF GamepadWidget::clampToCircle(const QPointF &p, qreal radius) const {
    const qreal len = qSqrt(p.x() * p.x() + p.y() * p.y());
    if (len <= radius || len == 0)
        return p;
    const qreal scale = radius / len;
    return QPointF(p.x() * scale, p.y() * scale);
}

void GamepadWidget::updateTopCircleFromPos(const QPoint &pos) {
    QPointF rel = QPointF(pos) - m_topCenter; // 相对中心
    rel = clampToCircle(rel, m_topRadius);
    m_knobXY = rel;

    m_nx = (m_knobXY.x() / m_topRadius);
    m_ny = (-m_knobXY.y() / m_topRadius);
    emit movedXY(qBound(-1.0, m_nx, 1.0), qBound(-1.0, m_ny, 1.0));
    update();
}

QRectF GamepadWidget::bottomCircleRect() const {
    return m_bottomCircle;
}

void GamepadWidget::updateBottomCircleFromPos(const QPoint &pos) {
    const QRectF r = bottomCircleRect();
    const qreal y = qBound(r.top(), (qreal)pos.y(), r.bottom());
    // 仅上下映射为 -1..1：上 -1，下 +1
    const qreal t = (y - r.top()) / r.height();
    m_knobZ = t * 2.0 - 1.0;

    m_nz = qBound(-1.0, m_knobZ, 1.0);
    emit movedZ(m_nz);
    update();
}

void GamepadWidget::mousePressEvent(QMouseEvent *e) {
    const QPoint pt = e->pos();
    // 若点击在指示器内，交由双击事件处理，不在此处消费
    if (pointInIndicator(pt)) {
        return;
    }
    if (QLineF(pt, m_topCenter).length() <= m_topRadius * 1.1) {
        m_pressedTop = true;
        updateTopCircleFromPos(pt);
        return;
    }
    if (bottomCircleRect().contains(pt)) {
        m_pressedBottom = true;
        updateBottomCircleFromPos(pt);
        return;
    }
}

void GamepadWidget::mouseMoveEvent(QMouseEvent *e) {
    const QPoint pt = e->pos();
    if (m_pressedTop) {
        updateTopCircleFromPos(pt);
    } else if (m_pressedBottom) {
        updateBottomCircleFromPos(pt);
    }
}

void GamepadWidget::mouseReleaseEvent(QMouseEvent *e) {
    Q_UNUSED(e);
    if (m_pressedTop) {
        m_pressedTop = false;
        m_topTouchId = -1;
        m_knobXY = QPointF(0, 0);
        m_nx = 0;
        m_ny = 0;
        emit releasedXY();
    }
    if (m_pressedBottom) {
        m_pressedBottom = false;
        m_bottomTouchId = -1;
        m_knobZ = 0.0;
        m_nz = 0;
        emit releasedZ();
    }
    update();
}

void GamepadWidget::mouseDoubleClickEvent(QMouseEvent *e) {
    const QPoint pt = e->pos();
    if (pointInIndicator(pt)) {
        if (QMessageBox::question(this, tr("询问"), tr("是否切换到WIFI模式？"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            sendRestart();
            e->accept();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(e);
}

bool GamepadWidget::event(QEvent *event) {
    if (event->type() == QEvent::TouchBegin || event->type() == QEvent::TouchUpdate || event->type() == QEvent::TouchEnd) {
        auto *te = static_cast<QTouchEvent *>(event);
        const auto points = te->points();
        // 如果触点在速度滑块区域内，交给基类处理，让子控件(QSlider)接管事件
        if (m_speedSlider) {
            for (const auto &pt : points) {
                const QPoint pos = pt.position().toPoint();
                if (m_speedSlider->geometry().contains(pos)) {
                    return QWidget::event(event);
                }
            }
        }
        for (const auto &pt : points) {
            const QPoint pos = pt.position().toPoint();
            const qint64 id = pt.id();
            switch (pt.state()) {
            case QEventPoint::Pressed: {
                // 指示器区域双击（双击）检测
                if (pointInIndicator(pos)) {
                    const qint64 now = QDateTime::currentMSecsSinceEpoch();
                    if (m_lastIndicatorTapMs > 0 && (now - m_lastIndicatorTapMs) <= kDoubleTapIntervalMs) {
                        // 双击成立
                        m_lastIndicatorTapMs = 0;
                        sendRestart();
                    } else {
                        m_lastIndicatorTapMs = now;
                        m_lastIndicatorTapPos = pos;
                    }
                    // 不继续传递给摇杆命中逻辑
                    break;
                }
                if (QLineF(pos, m_topCenter).length() <= m_topRadius * 1.1) {
                    if (m_topTouchId == -1)
                        m_topTouchId = id;
                    m_pressedTop = true;
                    updateTopCircleFromPos(pos);
                } else if (bottomCircleRect().contains(pos)) {
                    if (m_bottomTouchId == -1)
                        m_bottomTouchId = id;
                    m_pressedBottom = true;
                    updateBottomCircleFromPos(pos);
                }
                break;
            }
            case QEventPoint::Updated: {
                if (id == m_topTouchId && m_pressedTop) {
                    updateTopCircleFromPos(pos);
                } else if (id == m_bottomTouchId && m_pressedBottom) {
                    updateBottomCircleFromPos(pos);
                }
                break;
            }
            case QEventPoint::Released: {
                // 释放时不额外处理指示器逻辑（双击在 Pressed 阶段判定）
                if (id == m_topTouchId) {
                    m_topTouchId = -1;
                    if (m_pressedTop) {
                        m_pressedTop = false;
                        m_knobXY = QPointF(0, 0);
                        m_nx = 0;
                        m_ny = 0;
                        emit releasedXY();
                    }
                } else if (id == m_bottomTouchId) {
                    m_bottomTouchId = -1;
                    if (m_pressedBottom) {
                        m_pressedBottom = false;
                        m_knobZ = 0.0;
                        m_nz = 0;
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

void GamepadWidget::setConnected(bool connected) {
    if (m_connected == connected)
        return;
    m_connected = connected;
    if (m_connected) {
        // 已连接：显示常亮，设置为亮态
        m_blinkOn = true;
    } else {
        // 未连接：开始闪烁，起始为灭态
        m_blinkOn = false;
    }
    update();
}

void GamepadWidget::onMoveChanged() {
    if (!m_bt->isConnected())
        return;

    if (!m_bt->readyToSend())
        return; // 丢弃本次发送

    const QByteArray payload = "x:" + QByteArray::number(-1 * m_ny, 'f', 3) +
                               ",y:" + QByteArray::number(m_nx, 'f', 3) +
                               ",z:" + QByteArray::number(-1 * m_nz, 'f', 3) + "\n";
    m_bt->send(payload);
}

void GamepadWidget::onSpeedChanged(double percent) {
    if (!m_bt->isConnected())
        return;

    if (!m_bt->readyToSend())
        return; // 丢弃本次发送

    const QByteArray payload = "v:" + QByteArray::number(percent, 'f', 3) + "\n";
    m_bt->send(payload);
}

void GamepadWidget::sendRestart() {
    if (!m_bt->isConnected())
        return;

    if (!m_bt->readyToSend())
        return; // 丢弃本次发送

    const QByteArray payload = "r:r\n";
    m_bt->send(payload);
}
