#pragma once

#include "bluetoothclient.h"
#include "gamepadwidget.h"
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QTimer>
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onJoystickMoved(double x, double y);
    void onJoystickReleased();
    void onYawMoved(double z);
    void onYawReleased();

    void onBtConnected();
    void onBtDisconnected();

private:
    void setupUi();

private:
    BluetoothClient *m_bt{nullptr};
    GamepadWidget *m_gamepad{nullptr};
    QTimer *m_check_timer{nullptr};
};
