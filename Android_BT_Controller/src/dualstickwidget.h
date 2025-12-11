#pragma once

#include <QWidget>

class DualStickWidget : public QWidget {
    Q_OBJECT
public:
    explicit DualStickWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override { return QSize(300, 160); }
    QSize sizeHint() const override { return QSize(360, 180); }

signals:
    void moved(int x, int y); // -100..100
    void released();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void updateKnobFromPos(const QPoint &pos);
    QRectF leftTrack() const;
    QRectF rightTrack() const;

private:
    qreal m_radius{28.0};
    qreal m_leftX{0.0};  // -1..1 for X axis
    qreal m_rightX{0.0}; // -1..1 for Y axis (mapped from horizontal)
    bool m_pressed{false};
    int m_activeTrack{0}; // 1=left (X), 2=right (Y)
};
