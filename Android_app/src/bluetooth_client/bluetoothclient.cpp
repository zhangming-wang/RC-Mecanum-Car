#include "bluetoothclient.h"
#include <QBluetoothUuid>
#include <QDebug>

BluetoothClient::BluetoothClient(QObject *parent)
    : QObject(parent) {
    m_agent = new QBluetoothDeviceDiscoveryAgent(this);
    m_check_timer = new QTimer(this);
    m_ESP32_bluetooth_name = QString::fromUtf8(esp32_bluetooth_slave_name);

    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothClient::onDeviceDiscovered);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothClient::onDiscoveryFinished);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, [this](QBluetoothDeviceDiscoveryAgent::Error) { emit error(m_agent->errorString()); });

    connect(m_check_timer, &QTimer::timeout, this, &BluetoothClient::connectToESP32);
    connectToESP32();
    m_check_timer->setInterval(1000);
    m_check_timer->start();
}

void BluetoothClient::startDiscovery() {
    qDebug() << "Starting Bluetooth device discovery...";
    m_devices.clear();
    if (m_agent->isActive())
        m_agent->stop();
    m_agent->start();
}

void BluetoothClient::stopDiscovery() {
    if (m_agent->isActive())
        m_agent->stop();
}

bool BluetoothClient::isDiscovering() const {
    return m_agent->isActive();
}

void BluetoothClient::onDeviceDiscovered(const QBluetoothDeviceInfo &info) {
    // 仅保留经典蓝牙（非 BLE）
    if (!(info.coreConfigurations() & QBluetoothDeviceInfo::BaseRateCoreConfiguration))
        return;

    // 去重
    for (const auto &d : m_devices) {
        if (d.address() == info.address())
            return;
    }
    m_devices.push_back(info);
    qDebug() << "Discovered device:" << info.name() << info.address().toString();
    emit deviceFound(info);

    // 自动连接匹配名称的设备
    if (!m_ESP32_bluetooth_name.isEmpty() && info.name() == m_ESP32_bluetooth_name) {
        qDebug() << "Auto-connecting to target name:" << m_ESP32_bluetooth_name;
        // 停止继续扫描，直接连接
        if (m_agent->isActive())
            m_agent->stop();
        connectToAddress(info.address().toString());
    }
}

void BluetoothClient::onDiscoveryFinished() {
    qDebug() << "Bluetooth device discovery finished.";
    emit discoveryFinished();
}

void BluetoothClient::connectToAddress(const QString &address) {
    // 查找设备
    QBluetoothDeviceInfo chosen;
    bool found = false;
    for (const auto &d : m_devices) {
        if (d.address().toString() == address) {
            chosen = d;
            found = true;
            break;
        }
    }
    if (!found) {
        emit error(QStringLiteral("未找到设备: %1").arg(address));
        return;
    }

    if (m_socket && m_socket->state() != QBluetoothSocket::SocketState::UnconnectedState) {
        m_socket->disconnectFromService();
        m_socket.reset(nullptr);
    }

    m_socket.reset(new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol));
    connect(m_socket.data(), &QBluetoothSocket::connected, this, &BluetoothClient::onSocketConnected);
    connect(m_socket.data(), &QBluetoothSocket::disconnected, this, &BluetoothClient::onSocketDisconnected);
    connect(m_socket.data(), &QBluetoothSocket::readyRead, this, &BluetoothClient::onSocketReadyRead);
    connect(m_socket.data(), &QBluetoothSocket::errorOccurred,
            this, &BluetoothClient::onSocketError);

    // 通过 SPP UUID 连接
    const QBluetoothUuid spp(QBluetoothUuid::ServiceClassUuid::SerialPort);
    m_socket->connectToService(chosen.address(), spp);
}

void BluetoothClient::disconnectFromDevice() {
    if (m_socket) {
        m_socket->disconnectFromService();
    }
}

bool BluetoothClient::isConnected() const {
    return m_socket && m_socket->state() == QBluetoothSocket::SocketState::ConnectedState;
}

bool BluetoothClient::readyToSend() const {
    if (m_socket && m_socket->bytesToWrite() == 0)
        return true;
    else
        return false;
}

void BluetoothClient::send(const QByteArray &data) {
    if (!m_socket || m_socket->state() != QBluetoothSocket::SocketState::ConnectedState)
        return;
    m_socket->write(data);
}

void BluetoothClient::onSocketConnected() {
    emit connected();
}

void BluetoothClient::onSocketDisconnected() {
    emit disconnected();
}

void BluetoothClient::onSocketReadyRead() {
    if (!m_socket)
        return;
    const QByteArray data = m_socket->readAll();
    emit messageReceived(data);
}

void BluetoothClient::onSocketError(QBluetoothSocket::SocketError) {
    if (m_socket)
        emit error(m_socket->errorString());
}

void BluetoothClient::connectToESP32() {
    if (m_socket && m_socket->state() == QBluetoothSocket::SocketState::ConnectedState)
        return;

    qDebug() << "attempting to connect to " << m_ESP32_bluetooth_name << "......";

    for (const auto &d : m_devices) {
        if (d.name() == m_ESP32_bluetooth_name) {
            connectToAddress(d.address().toString());
            return;
        }
    }
    if (!m_agent->isActive())
        startDiscovery();
}
