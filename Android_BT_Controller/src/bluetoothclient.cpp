#include "bluetoothclient.h"
#include <QBluetoothUuid>
#include <QDebug>

BluetoothClient::BluetoothClient(QObject *parent)
    : QObject(parent) {
    m_agent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BluetoothClient::onDeviceDiscovered);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BluetoothClient::onDiscoveryFinished);
    connect(m_agent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
            this, [this](QBluetoothDeviceDiscoveryAgent::Error) { emit error(m_agent->errorString()); });
}

void BluetoothClient::startDiscovery() {
    m_devices.clear();
    if (m_agent->isActive())
        m_agent->stop();
    m_agent->setInquiryType(QBluetoothDeviceDiscoveryAgent::GeneralUnlimitedInquiry);
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
}

void BluetoothClient::onDiscoveryFinished() {
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

    if (m_socket && m_socket->state() != QBluetoothSocket::UnconnectedState) {
        m_socket->disconnectFromService();
        m_socket.reset(nullptr);
    }

    m_socket.reset(new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol));
    connect(m_socket.data(), &QBluetoothSocket::connected, this, &BluetoothClient::onSocketConnected);
    connect(m_socket.data(), &QBluetoothSocket::disconnected, this, &BluetoothClient::onSocketDisconnected);
    connect(m_socket.data(), &QBluetoothSocket::readyRead, this, &BluetoothClient::onSocketReadyRead);
    connect(m_socket.data(), QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
            this, &BluetoothClient::onSocketError);

    // 通过 SPP UUID 连接
    const QBluetoothUuid spp(QBluetoothUuid::SerialPort);
    m_socket->connectToService(chosen.address(), spp);
}

void BluetoothClient::disconnectFromDevice() {
    if (m_socket) {
        m_socket->disconnectFromService();
    }
}

bool BluetoothClient::isConnected() const {
    return m_socket && m_socket->state() == QBluetoothSocket::ConnectedState;
}

void BluetoothClient::send(const QByteArray &data) {
    if (!m_socket || m_socket->state() != QBluetoothSocket::ConnectedState)
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
