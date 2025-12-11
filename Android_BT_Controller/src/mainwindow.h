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
    void onScan();
    void onConnect();
    void onDisconnect();

    void onDeviceFound(const QBluetoothDeviceInfo &info);
    void onDiscoveryFinished();

    void onBtConnected();
    void onBtDisconnected();
    void onBtError(const QString &message);

    void onJoystickMoved(double x, double y);
    void onJoystickReleased();
    void onYawMoved(double z);
    void onOverallSpeedChanged(int pct);
    void onAutoReconnectTick();

private:
    void setupUi();
    void refreshDeviceCombo();
    void updateStatusLabel();

private:
    BluetoothClient *m_bt{nullptr};

    QComboBox *m_deviceCombo{nullptr};
    QPushButton *m_btnScan{nullptr};
    QPushButton *m_btnConnect{nullptr};
    QPushButton *m_btnDisconnect{nullptr};
    QLabel *m_status{nullptr}, *m_spdLabel{nullptr};
    QSlider *m_speed{nullptr}; // 线速度百分比（整体比例）
    GamepadWidget *m_gamepad{nullptr};
    QTimer *m_reconnectTimer{nullptr};
    QString m_lastAddress;

    QList<QBluetoothDeviceInfo> m_devicesCache;
    int m_overallPct{60};
    double m_lastX{0.0};
    double m_lastY{0.0};
    double m_lastZ{0.0};
};
