#include "mainwindow.h"
#include "../common/settings.h"
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUi();

    m_bt = new BluetoothClient(this);
    connect(m_bt, &BluetoothClient::connected, this, &MainWindow::onBtConnected);
    connect(m_bt, &BluetoothClient::disconnected, this, &MainWindow::onBtDisconnected);
    connect(m_bt, &BluetoothClient::error, this, [](const QString &msg) {
        qWarning() << "Bluetooth error:" << msg;
    });
    connect(m_bt, &BluetoothClient::messageReceived, this, [](const QByteArray &data) {
        qDebug() << "Received data:" << data;
    });

    // 启动后尝试连接指定的 ESP32 蓝牙名称
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    m_gamepad = new GamepadWidget();

    connect(m_gamepad, &GamepadWidget::movedXY, this, &MainWindow::onJoystickMoved);
    connect(m_gamepad, &GamepadWidget::releasedXY, this, &MainWindow::onJoystickReleased);
    connect(m_gamepad, &GamepadWidget::movedZ, this, &MainWindow::onYawMoved);
    connect(m_gamepad, &GamepadWidget::releasedZ, this, &MainWindow::onYawReleased);

    root->addWidget(m_gamepad);
    root->setContentsMargins(0, 0, 0, 0);
}

void MainWindow::onBtConnected() {
}

void MainWindow::onBtDisconnected() {
}

void MainWindow::onJoystickMoved(double x, double y) {
    if (!m_bt->isConnected())
        return;

    const QByteArray payload;
    m_bt->send(payload);
}

void MainWindow::onJoystickReleased() {
    if (!m_bt->isConnected())
        return;

    const QByteArray payload;
    m_bt->send(payload);
}

void MainWindow::onYawMoved(double z) {
    if (!m_bt->isConnected())
        return;
    const QByteArray payload;
    m_bt->send(payload);
}

void MainWindow::onYawReleased() {
    if (!m_bt->isConnected())
        return;
    const QByteArray payload;
    m_bt->send(payload);
}