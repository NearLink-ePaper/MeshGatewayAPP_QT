#include "blemanager.h"
#include <QBluetoothUuid>
#include <QDebug>
#include <QCoreApplication>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QPermissions>
#endif

const QBluetoothUuid BleManager::SERVICE_UUID(QBluetoothUuid(quint16(0xFFE0)));
const QBluetoothUuid BleManager::TX_CHAR_UUID(QBluetoothUuid(quint16(0xFFE1)));
const QBluetoothUuid BleManager::RX_CHAR_UUID(QBluetoothUuid(quint16(0xFFE2)));
const QBluetoothUuid BleManager::CCCD_UUID(QBluetoothUuid(quint16(0x2902)));

BleManager::BleManager(QObject *parent)
    : QObject(parent)
{
    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_discoveryAgent->setLowEnergyDiscoveryTimeout(SCAN_TIMEOUT_MS);

    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BleManager::onDeviceDiscovered);
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BleManager::onScanFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BleManager::onScanError);
#else
    connect(m_discoveryAgent,
            QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
            this, &BleManager::onScanError);
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated,
            this, [this](const QBluetoothDeviceInfo &info, auto /*fields*/) {
        for (int i = 0; i < m_scannedDevices.size(); ++i) {
            if (m_scannedDevices[i].name == info.name()) {
                m_scannedDevices[i].rssi = info.rssi();
                emit scannedDevicesChanged();
                break;
            }
        }
    });
#endif

    m_scanTimer.setSingleShot(true);
    connect(&m_scanTimer, &QTimer::timeout, this, &BleManager::stopScan);

    setDebugInfo(tr("Ready"));
}

BleManager::~BleManager()
{
    disconnectDevice();
}

void BleManager::setConnState(ConnState s)
{
    if (m_connState == s) return;
    m_connState = s;
    emit connStateChanged(s);
}

void BleManager::setDebugInfo(const QString &info)
{
    m_debugInfo = info;
    qDebug() << "[BLE]" << info;
    emit debugInfoChanged(info);
}

// ─── 扫描 ───────────────────────────────────────────────

void BleManager::startScan()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QBluetoothPermission btPerm;
    switch (qApp->checkPermission(btPerm)) {
    case Qt::PermissionStatus::Granted:
        doStartScan();
        break;
    case Qt::PermissionStatus::Undetermined:
        qApp->requestPermission(btPerm, this, [this](const QPermission &permission) {
            if (permission.status() == Qt::PermissionStatus::Granted)
                doStartScan();
            else {
                setConnState(Disconnected);
                setDebugInfo(tr("Bluetooth permission denied"));
            }
        });
        break;
    case Qt::PermissionStatus::Denied:
        setConnState(Disconnected);
        setDebugInfo(tr("Bluetooth permission denied. Please enable in Settings."));
        break;
    }
#else
    doStartScan();
#endif
}

void BleManager::doStartScan()
{
    m_scannedDevices.clear();
    emit scannedDevicesChanged();
    setConnState(Scanning);
    setDebugInfo(tr("Scanning... (12s)"));

    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    m_scanTimer.start(SCAN_TIMEOUT_MS);
}

void BleManager::stopScan()
{
    m_scanTimer.stop();
    if (m_discoveryAgent->isActive())
        m_discoveryAgent->stop();

    if (m_connState == Scanning) {
        setConnState(Disconnected);
        setDebugInfo(tr("Scan finished, found %1 device(s)").arg(m_scannedDevices.size()));
    }
}

void BleManager::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
        return;

    QString name = info.name();
    if (name.isEmpty()) return;
    if (!name.toUpper().startsWith("SLE_GW_")) return;

    ScannedDevice dev;
    dev.name       = name;
    dev.address    = info.address().toString();
#ifdef Q_OS_IOS
    // iOS 不暴露 MAC，使用 deviceUuid
    dev.address = info.deviceUuid().toString();
#endif
    dev.rssi       = info.rssi();
    dev.deviceInfo = info;

    // 按名称去重
    bool found = false;
    for (int i = 0; i < m_scannedDevices.size(); ++i) {
        if (m_scannedDevices[i].name == dev.name) {
            m_scannedDevices[i] = dev;
            found = true;
            break;
        }
    }
    if (!found)
        m_scannedDevices.append(dev);

    emit scannedDevicesChanged();
    setDebugInfo(tr("Scanning... found %1 gateway(s)").arg(m_scannedDevices.size()));
}

void BleManager::onScanFinished()
{
    if (m_connState == Scanning) {
        setConnState(Disconnected);
        setDebugInfo(tr("Scan finished, found %1 device(s)").arg(m_scannedDevices.size()));
    }
}

void BleManager::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    setConnState(Disconnected);
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    if (error == QBluetoothDeviceDiscoveryAgent::MissingPermissionsError) {
        setDebugInfo(tr("Bluetooth permission required. Please grant Bluetooth access in system settings."));
        return;
    }
#else
    Q_UNUSED(error)
#endif
    setDebugInfo(tr("Scan failed: %1").arg(m_discoveryAgent->errorString()));
}

// ─── 连接 ───────────────────────────────────────────────

void BleManager::connectToDevice(int index)
{
    if (index < 0 || index >= m_scannedDevices.size()) return;

    stopScan();
    const ScannedDevice &dev = m_scannedDevices.at(index);
    setConnState(Connecting);
    m_deviceName = dev.name;
    emit deviceNameChanged(m_deviceName);

    // 清理旧连接
    if (m_controller) {
        m_controller->disconnectFromDevice();
        m_controller->deleteLater();
        m_controller = nullptr;
    }
    m_service = nullptr;

    m_controller = QLowEnergyController::createCentral(dev.deviceInfo, this);

    connect(m_controller, &QLowEnergyController::connected,
            this, &BleManager::onControllerConnected);
    connect(m_controller, &QLowEnergyController::disconnected,
            this, &BleManager::onControllerDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
    connect(m_controller, &QLowEnergyController::errorOccurred,
            this, &BleManager::onControllerError);
#else
    connect(m_controller,
            QOverload<QLowEnergyController::Error>::of(&QLowEnergyController::error),
            this, &BleManager::onControllerError);
#endif
    connect(m_controller, &QLowEnergyController::serviceDiscovered,
            this, &BleManager::onServiceDiscovered);
    connect(m_controller, &QLowEnergyController::discoveryFinished,
            this, &BleManager::onServiceDiscoveryFinished);

    setDebugInfo(tr("Connecting to %1 ...").arg(dev.name));
    m_controller->connectToDevice();
}

void BleManager::disconnectDevice()
{
    if (m_service) {
        m_service->deleteLater();
        m_service = nullptr;
    }
    if (m_controller) {
        m_controller->disconnectFromDevice();
        m_controller->deleteLater();
        m_controller = nullptr;
    }
    m_rxChar = QLowEnergyCharacteristic();
    m_txChar = QLowEnergyCharacteristic();
    setConnState(Disconnected);
    m_deviceName.clear();
    emit deviceNameChanged(m_deviceName);
}

void BleManager::updateDeviceName(const QString &name)
{
    m_deviceName = name;
    emit deviceNameChanged(name);
}

// ─── GATT 回调 ──────────────────────────────────────────

void BleManager::onControllerConnected()
{
    setConnState(Connected);
    setDebugInfo(tr("Connected, discovering services..."));
    m_controller->discoverServices();
}

void BleManager::onControllerDisconnected()
{
    setConnState(Disconnected);
    m_deviceName.clear();
    emit deviceNameChanged(m_deviceName);
    setDebugInfo(tr("Disconnected"));
}

void BleManager::onControllerError(QLowEnergyController::Error error)
{
    Q_UNUSED(error)
    setDebugInfo(tr("Connection error: %1").arg(m_controller->errorString()));
    if (m_connState != Connected) {
        setConnState(Disconnected);
    }
}

void BleManager::onServiceDiscovered(const QBluetoothUuid &serviceUuid)
{
    qDebug() << "[BLE] Service discovered:" << serviceUuid.toString();
}

void BleManager::onServiceDiscoveryFinished()
{
    setDebugInfo(tr("Service discovery finished, setting up FFE0..."));
    setupService();
}

void BleManager::setupService()
{
    if (!m_controller) return;

    m_service = m_controller->createServiceObject(SERVICE_UUID, this);
    if (!m_service) {
        setDebugInfo(tr("FFE0 service not found!"));
        return;
    }

    connect(m_service, &QLowEnergyService::stateChanged,
            this, &BleManager::onServiceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged,
            this, &BleManager::onCharacteristicChanged);
    connect(m_service, &QLowEnergyService::descriptorWritten,
            this, &BleManager::onDescriptorWritten);

    m_service->discoverDetails();
}

void BleManager::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (state != QLowEnergyService::RemoteServiceDiscovered)
#else
    if (state != QLowEnergyService::ServiceDiscovered)
#endif
        return;

    // 获取特征
    m_txChar = m_service->characteristic(TX_CHAR_UUID);
    m_rxChar = m_service->characteristic(RX_CHAR_UUID);

    if (!m_txChar.isValid()) {
        setDebugInfo(tr("FFE1 (Notify) characteristic not found!"));
        return;
    }
    if (!m_rxChar.isValid()) {
        setDebugInfo(tr("FFE2 (Write) characteristic not found!"));
        return;
    }

    // 订阅 Notify
    // CCCD 值: 0x0100 = Enable Notification, 0x0200 = Enable Indication
    static const QByteArray CCCD_NOTIFY  = QByteArray::fromHex("0100");
    static const QByteArray CCCD_INDICATE = QByteArray::fromHex("0200");

    QLowEnergyDescriptor cccd = m_txChar.descriptor(CCCD_UUID);
    if (cccd.isValid()) {
        if (m_txChar.properties() & QLowEnergyCharacteristic::Indicate) {
            m_service->writeDescriptor(cccd, CCCD_INDICATE);
        } else {
            m_service->writeDescriptor(cccd, CCCD_NOTIFY);
        }
        setDebugInfo(tr("Writing CCCD for notifications..."));
    } else {
        setDebugInfo(tr("CCCD not found on FFE1, notifications may not work"));
    }
}

void BleManager::onCharacteristicChanged(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() == TX_CHAR_UUID) {
        setDebugInfo(tr("Received: %1").arg(MeshProtocol::toHexString(value)));
        handleNotify(value);
    }
}

void BleManager::onDescriptorWritten(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    Q_UNUSED(value)
    if (d.uuid() == CCCD_UUID) {
        setDebugInfo(tr("CCCD subscription successful"));
    }
}

void BleManager::handleNotify(const QByteArray &data)
{
    UpstreamMessage msg = MeshProtocol::parseNotification(data);
    if (msg.type == UpstreamMessage::Unknown) {
        // 解析失败仍然通知 UI 显示原始数据
        msg.type    = UpstreamMessage::DataFromNode;
        msg.srcAddr = 0;
        msg.payload = data;
    }
    emit messageReceived(msg);
}

// ─── 发送 ───────────────────────────────────────────────

bool BleManager::sendRaw(const QByteArray &data)
{
    if (!m_service || !m_rxChar.isValid()) return false;
    m_service->writeCharacteristic(m_rxChar, data, QLowEnergyService::WriteWithResponse);
    return true;
}

bool BleManager::sendToNode(quint16 dstAddr, const QByteArray &payload)
{
    return sendRaw(MeshProtocol::buildUnicast(dstAddr, payload));
}

bool BleManager::broadcast(const QByteArray &payload)
{
    return sendRaw(MeshProtocol::buildBroadcast(payload));
}

bool BleManager::queryTopology()
{
    return sendRaw(MeshProtocol::buildTopoQuery());
}
