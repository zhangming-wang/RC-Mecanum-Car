#pragma once

#include "../bluetooth_client/bluetoothclient.h"
#include <QDateTime>
#include <QEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QShowEvent>
#include <QSlider>
#include <QTimer>
#include <QTouchEvent>
#include <QWidget>
#include <QtMath>

class GamepadWidget : public QWidget {
    Q_OBJECT
public:
    explicit GamepadWidget(QWidget *parent = nullptr);
    // 连接状态控制
    void setConnected(bool connected);
    bool isConnected() const { return m_connected; }

signals:
    void movedXY(double x, double y); // -1.0..1.0
    void releasedXY();
    void movedZ(double z); // -1.0..1.0（下方圆形摇杆，垂直）
    void releasedZ();
    void speedChanged(double percent); // 0..1.0 中间滑块比例

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    bool event(QEvent *event) override;

private slots:
    void onSpeedChanged(double percent);
    void onMoveChanged();
    void onIndicatorSingleTap();

private:
    // 上方圆形摇杆（XY）
    QPointF clampToCircle(const QPointF &p, qreal radius) const;
    void updateTopCircleFromPos(const QPoint &pos);

    // 下方圆形摇杆（仅控制Z，垂直方向）
    QRectF bottomCircleRect() const; // 下方圆形区域
    void updateBottomCircleFromPos(const QPoint &pos);
    QRectF indicatorRect() const;                  // 右侧状态指示器区域
    bool pointInIndicator(const QPoint &pt) const; // 命中测试
    void sendRestart();                            // 下发重启命令（切 WiFi）
    void sendSwitchToPs3();                        // 下发切换 PS3 命令

    // 布局尺寸
    QRectF m_topArea;    // 上方圆形摇杆区域
    QRectF m_bottomArea; // 下方圆形摇杆区域
    // 固定控件尺寸 + 中间expanding留白：形状不随容器高度/宽度变化
    qreal m_topFixedW{200.0};    // 上方圆目标宽度
    qreal m_topFixedH{200.0};    // 上方圆目标高度
    qreal m_bottomFixedW{200.0}; // 下方圆目标宽度
    qreal m_bottomFixedH{200.0}; // 下方圆目标高度

    // 上方圆形摇杆状态（XY）
    QPointF m_topCenter;
    qreal m_topRadius{90.0};
    QPointF m_knobXY;              // 相对中心
    double m_nx = 0.0, m_ny = 0.0; // -1..1
    bool m_pressedTop{false};
    qint64 m_topTouchId{-1};

    // 下方圆形摇杆状态（Z轴）
    QRectF m_bottomCircle;
    qreal m_knobZ{0.0}; // -1..1 垂直方向（上-1，下+1）
    double m_nz = 0.0;  // -1..1
    bool m_pressedBottom{false};
    qint64 m_bottomTouchId{-1};

    // 中间速度滑块
    QSlider *m_speedSlider{nullptr};
    // 布局常量（可配置）
    qreal m_edgeMargin{50.0};
    qreal m_spacingInside{20.0};

    // 右侧连接状态指示器
    bool m_connected{false};
    bool m_blinkOn{false};
    QTimer m_blinkTimer;
    int m_indicatorSize{60};
    int m_indicatorPad{0};
    QColor m_indicatorOnColor{QColor(0, 200, 70)};
    QColor m_indicatorOffColor{QColor(110, 110, 110)};

    // 指示器单击/双击检测
    qint64 m_lastIndicatorTapMs{0};
    QPoint m_lastIndicatorTapPos{0, 0};
    QTimer m_singleTapTimer; // 单击等待定时器（区分双击）

    BluetoothClient *m_bt{nullptr};
};
