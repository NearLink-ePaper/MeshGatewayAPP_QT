#ifndef BLEMANAGER_H
#define BLEMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>
#include <QLowEnergyDescriptor>

#include "meshprotocol.h"

/**
 * BLE 通信管理器
 * 封装 BLE 扫描、连接、GATT 读写、Notify 订阅
 * 使用 Qt Bluetooth 模块，支持 Windows/macOS/Linux/iOS/Android
 */
class BleManager : public QObject
{
    Q_OBJECT

public:
    enum ConnState {
        Disconnected,
        Scanning,
        Connecting,
        Connected
    };
    Q_ENUM(ConnState)

    struct ScannedDevice {
        QString              name;
        QString              address;
        int                  rssi;
        QBluetoothDeviceInfo deviceInfo;
    };

    explicit BleManager(QObject *parent = nullptr);
    ~BleManager() override;

    ConnState connState() const { return m_connState; }
    QList<ScannedDevice> scannedDevices() const { return m_scannedDevices; }
    QString deviceName() const { return m_deviceName; }
    QString debugInfo() const { return m_debugInfo; }

public slots:
    void startScan();
    void stopScan();
    void connectToDevice(int index);
    void disconnectDevice();
    void updateDeviceName(const QString &name);

    bool sendToNode(quint16 dstAddr, const QByteArray &payload);
    bool broadcast(const QByteArray &payload);
    bool queryTopology();

signals:
    void connStateChanged(BleManager::ConnState state);
    void scannedDevicesChanged();
    void deviceNameChanged(const QString &name);
    void debugInfoChanged(const QString &info);
    void messageReceived(const UpstreamMessage &msg);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onScanFinished();
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void onControllerConnected();
    void onControllerDisconnected();
    void onControllerError(QLowEnergyController::Error error);
    void onServiceDiscovered(const QBluetoothUuid &serviceUuid);
    void onServiceDiscoveryFinished();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onDescriptorWritten(const QLowEnergyDescriptor &d, const QByteArray &value);

private:
    void setConnState(ConnState s);
    void setDebugInfo(const QString &info);
    void doStartScan();
    bool sendRaw(const QByteArray &data);
    void handleNotify(const QByteArray &data);
    void setupService();

    // GATT UUIDs
    static const QBluetoothUuid SERVICE_UUID;
    static const QBluetoothUuid TX_CHAR_UUID;   // Notify: 网关→手机 (0xFFE1)
    static const QBluetoothUuid RX_CHAR_UUID;   // Write:  手机→网关 (0xFFE2)
    static const QBluetoothUuid CCCD_UUID;

    static constexpr int SCAN_TIMEOUT_MS = 12000;

    ConnState                       m_connState = Disconnected;
    QList<ScannedDevice>            m_scannedDevices;
    QString                         m_deviceName;
    QString                         m_debugInfo;

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController           *m_controller     = nullptr;
    QLowEnergyService              *m_service        = nullptr;
    QLowEnergyCharacteristic        m_rxChar;         // Write characteristic
    QLowEnergyCharacteristic        m_txChar;         // Notify characteristic
    QTimer                          m_scanTimer;
};

#endif // BLEMANAGER_H
