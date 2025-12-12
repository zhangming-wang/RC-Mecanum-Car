#pragma once

#include "../common/settings.h" // For esp32_bluetooth_slave_name
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothSocket>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

class BluetoothClient : public QObject {
    Q_OBJECT
public:
    explicit BluetoothClient(QObject *parent = nullptr);

    void startDiscovery();
    void stopDiscovery();

    QList<QBluetoothDeviceInfo> devices() const { return m_devices; }

    void connectToAddress(const QString &address);
    void disconnectFromDevice();

    bool isConnected() const;
    bool isDiscovering() const;

public slots:
    void send(const QByteArray &data);

signals:
    void deviceFound(const QBluetoothDeviceInfo &info);
    void discoveryFinished();
    void connected();
    void disconnected();
    void error(const QString &message);
    void messageReceived(const QByteArray &data);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onDiscoveryFinished();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QBluetoothSocket::SocketError err);

private:
    QBluetoothDeviceDiscoveryAgent *m_agent{nullptr};
    QList<QBluetoothDeviceInfo> m_devices;
    QScopedPointer<QBluetoothSocket> m_socket;
    QString m_ESP32_bluetooth_name;

    QTimer *m_check_timer;

    void connectToESP32();
};
