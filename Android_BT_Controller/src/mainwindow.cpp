#include "mainwindow.h"

#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    m_bt = new BluetoothClient(this);

    connect(m_bt, &BluetoothClient::deviceFound, this, &MainWindow::onDeviceFound);
    connect(m_bt, &BluetoothClient::discoveryFinished, this, &MainWindow::onDiscoveryFinished);
    connect(m_bt, &BluetoothClient::connected, this, &MainWindow::onBtConnected);
    connect(m_bt, &BluetoothClient::disconnected, this, &MainWindow::onBtDisconnected);
    connect(m_bt, &BluetoothClient::error, this, &MainWindow::onBtError);

    setupUi();
}

void MainWindow::setupUi() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(9, 9, 9, 9);
    root->setSpacing(6);

    // 连接区域
    auto *connLayout = new QHBoxLayout();
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_btnScan = new QPushButton(tr("扫描"));
    m_btnConnect = new QPushButton(tr("连接"));
    m_btnDisconnect = new QPushButton(tr("断开"));

    connLayout->addWidget(m_btnScan);
    connLayout->addWidget(m_deviceCombo);
    connLayout->addWidget(m_btnConnect);
    connLayout->addWidget(m_btnDisconnect);

    root->addLayout(connLayout);

    // 速度滑块
    auto speedLayout = new QHBoxLayout();
    auto labelSpeed = new QLabel(tr("速度:"));
    speedLayout->addWidget(labelSpeed);
    m_speed = new QSlider(Qt::Horizontal);
    m_speed->setRange(0, 100);
    m_speed->setValue(m_overallPct);
    speedLayout->addWidget(m_speed);
    m_spdLabel = new QLabel(QString::number(m_overallPct) + '%');
    speedLayout->addWidget(m_spdLabel);
    root->addLayout(speedLayout);

    auto padLayout = new QVBoxLayout();
    m_gamepad = new GamepadWidget();
    m_gamepad->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    padLayout->addWidget(m_gamepad);
    root->addLayout(padLayout);

    m_status = new QLabel(tr("未连接"));
    root->addWidget(m_status);

    connect(m_btnScan, &QPushButton::clicked, this, &MainWindow::onScan);
    connect(m_btnConnect, &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnect);

    connect(m_gamepad, &GamepadWidget::movedXY, this, &MainWindow::onJoystickMoved);
    connect(m_gamepad, &GamepadWidget::releasedXY, this, &MainWindow::onJoystickReleased);
    connect(m_gamepad, &GamepadWidget::movedZ, this, &MainWindow::onYawMoved);
    connect(m_gamepad, &GamepadWidget::releasedZ, this, &MainWindow::onJoystickReleased);
    connect(m_speed, &QSlider::valueChanged, this, &MainWindow::onOverallSpeedChanged);

    // 自动重连定时器
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(3000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &MainWindow::onAutoReconnectTick);

    setWindowTitle(tr("ESP32 手柄控制"));
}

void MainWindow::onScan() {
    m_devicesCache.clear();
    m_deviceCombo->clear();
    m_status->setText(tr("正在扫描..."));
    m_bt->startDiscovery();
}

void MainWindow::onConnect() {
    if (m_deviceCombo->count() == 0)
        return;
    const QString addr = m_deviceCombo->currentData().toString();
    if (addr.isEmpty())
        return;
    m_status->setText(tr("连接中..."));
    m_lastAddress = addr;
    m_bt->connectToAddress(addr);
}

void MainWindow::onDisconnect() {
    m_bt->disconnectFromDevice();
}

void MainWindow::onDeviceFound(const QBluetoothDeviceInfo &info) {
    m_devicesCache.push_back(info);
    refreshDeviceCombo();
}

void MainWindow::onDiscoveryFinished() {
    if (m_devicesCache.isEmpty()) {
        m_deviceCombo->clear();
    }
}

void MainWindow::onBtConnected() {
    updateStatusLabel();
    m_reconnectTimer->stop();
}

void MainWindow::onBtDisconnected() {
    updateStatusLabel();
    // 自动重连（如果之前选择过地址）
    if (!m_lastAddress.isEmpty()) {
        m_reconnectTimer->start();
    }
}

void MainWindow::onBtError(const QString &message) {
    m_status->setText(tr("错误: ") + message);
}

void MainWindow::onJoystickMoved(double x, double y) {
    if (!m_bt->isConnected())
        return;
    m_lastX = x;
    m_lastY = y;
    const int pct = m_speed->value();
    const double z = m_lastZ;
    // 协议仍使用 -100..100，内部改为 -1..1 -> 乘以100
    const QByteArray payload = QByteArray::number((int)qRound(x * 100)) + ',' + QByteArray::number((int)qRound(y * 100)) + ',' + QByteArray::number((int)qRound(z * 100)) + ',' + QByteArray::number(pct) + '\n';
    m_bt->send(payload);
}

void MainWindow::onJoystickReleased() {
    if (!m_bt->isConnected())
        return;
    // 松开置 0
    m_lastX = 0.0;
    m_lastY = 0.0;
    const int pct = m_speed->value();
    const double z = m_lastZ;
    const QByteArray payload = QByteArray("0,0,") + QByteArray::number((int)qRound(z * 100)) + ',' + QByteArray::number(pct) + '\n';
    m_bt->send(payload);
}

void MainWindow::onYawMoved(double z) {
    m_lastZ = z;
    if (!m_bt->isConnected())
        return;
    const int pct = m_speed->value();
    const QByteArray payload = QByteArray::number((int)qRound(m_lastX * 100)) + ',' + QByteArray::number((int)qRound(m_lastY * 100)) + ',' + QByteArray::number((int)qRound(z * 100)) + ',' + QByteArray::number(pct) + '\n';
    m_bt->send(payload);
}

void MainWindow::onOverallSpeedChanged(int pct) {
    m_overallPct = pct;
    m_spdLabel->setText(QString::number(pct / (double)m_speed->maximum(), 'f', 2) + '%');
    if (!m_bt->isConnected())
        return;
    const QByteArray payload = QByteArray::number((int)qRound(m_lastX * 100)) + ',' + QByteArray::number((int)qRound(m_lastY * 100)) + ',' + QByteArray::number((int)qRound(m_lastZ * 100)) + ',' + QByteArray::number(pct) + '\n';
    m_bt->send(payload);
}

void MainWindow::onAutoReconnectTick() {
    if (m_bt->isConnected() || m_lastAddress.isEmpty())
        return;
    m_status->setText(tr("自动重连中..."));
    m_bt->connectToAddress(m_lastAddress);
}

void MainWindow::updateStatusLabel() {
    m_status->setText(m_bt->isConnected() ? tr("已连接") : tr("未连接"));
}

void MainWindow::refreshDeviceCombo() {
    const int sel = m_deviceCombo->currentIndex();
    const QString selData = m_deviceCombo->currentData().toString();

    m_deviceCombo->clear();
    for (const auto &d : m_devicesCache) {
        const QString name = d.name().isEmpty() ? QStringLiteral("<未知设备>") : d.name();
        const QString addr = d.address().toString();
        m_deviceCombo->addItem(name + " (" + addr + ")", addr);
    }

    // 维持选择
    int idx = m_deviceCombo->findData(selData);
    if (idx < 0 && sel >= 0 && sel < m_deviceCombo->count())
        idx = sel;
    if (idx >= 0)
        m_deviceCombo->setCurrentIndex(idx);
}
