#pragma once

#include <QWidget>

class YawWidget : public QWidget {
    Q_OBJECT
public:
    explicit YawWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override { return QSize(220, 100); }
    QSize sizeHint() const override { return QSize(260, 120); }

signals:
    void moved(int z); // -100..100 左负右正
    void released();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void updateKnobFromPos(const QPoint &pos);

private:
    QRectF m_trackRect;   // 水平轨道
    qreal m_radius{40.0}; // 旋钮半径
    qreal m_knobX{0.0};   // 相对中心的X偏移（-1..1）
    bool m_pressed{false};
};
