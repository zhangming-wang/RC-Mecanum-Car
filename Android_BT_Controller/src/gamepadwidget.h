#pragma once

#include <QWidget>

class GamepadWidget : public QWidget {
    Q_OBJECT
public:
    explicit GamepadWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override { return QSize(540, 240); }
    QSize sizeHint() const override { return QSize(680, 300); }

signals:
    void movedXY(double x, double y); // -1.0..1.0
    void releasedXY();
    void movedZ(double z); // -1.0..1.0（水平，胶囊形）
    void releasedZ();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    // 左侧圆形摇杆
    QPointF clampToCircle(const QPointF &p, qreal radius) const;
    void updateCircleFromPos(const QPoint &pos);

    // 右侧胶囊水平摇杆
    QRectF capsuleRect() const; // 胶囊区域（圆角矩形）
    void updateCapsuleFromPos(const QPoint &pos);

private:
    // 布局尺寸
    QRectF m_leftArea;  // 左侧区域（圆形摇杆）
    QRectF m_rightArea; // 右侧区域（胶囊摇杆）
    // 固定控件尺寸 + 中间expanding留白：形状不随容器高度/宽度变化
    qreal m_leftFixedW{200.0};  // 左控件目标宽度
    qreal m_leftFixedH{200.0};  // 左控件目标高度（圆形区域）
    qreal m_rightFixedW{200.0}; // 右控件目标宽度
    qreal m_rightFixedH{100.0}; // 右控件目标高度（胶囊区域）

    // 左侧圆形摇杆状态
    QPointF m_centerLeft;
    qreal m_radiusLeft{90.0};
    QPointF m_knobLeft; // 相对中心
    bool m_pressedLeft{false};

    // 右侧胶囊摇杆状态
    QRectF m_capsule;
    qreal m_capsuleRadius{30.0};
    qreal m_capsuleMargin{20.0};
    qreal m_knobZ{0.0}; // -1..1 映射到 -100..100
    bool m_pressedRight{false};
};
