#pragma once

#include <QWidget>

class YawCircleWidget : public QWidget {
    Q_OBJECT
public:
    explicit YawCircleWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override { return QSize(180, 180); }
    QSize sizeHint() const override { return QSize(220, 220); }

signals:
    void moved(int z); // -100..100，左右为负/正
    void released();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QPointF clampToCircle(const QPointF &p, qreal radius) const;
    void updateKnobFromPos(const QPoint &pos);

private:
    QPointF m_center;
    QPointF m_knobPos; // 相对中心坐标，仅使用x分量
    qreal m_radius{90.0};
    bool m_pressed{false};
};
