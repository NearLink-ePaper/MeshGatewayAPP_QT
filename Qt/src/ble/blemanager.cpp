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

    // RSSI 刷新: 每 2 秒重启扫描，强制 CoreBluetooth 重新发送 RSSI
    m_rssiRefreshTimer.setInterval(2000);
    connect(&m_rssiRefreshTimer, &QTimer::timeout, this, &BleManager::restartScanForRssi);

    // 拓扑查询超时
    m_topoQueryTimer.setSingleShot(true);
    m_topoQueryTimer.setInterval(5000);
    connect(&m_topoQueryTimer, &QTimer::timeout, this, [this]() {
        m_topoQuerying = false;
        emit topoQueryingChanged(false);
    });

    // 图片发送定时器
    m_imgAckTimer.setSingleShot(true);
    connect(&m_imgAckTimer, &QTimer::timeout, this, &BleManager::onImageAckTimeout);

    m_imgResultTimer.setSingleShot(true);
    connect(&m_imgResultTimer, &QTimer::timeout, this, &BleManager::onImageResultTimeout);

    m_imgFastPaceTimer.setSingleShot(true);
    connect(&m_imgFastPaceTimer, &QTimer::timeout, this, &BleManager::imgSendNextFastPaced);

    m_imgFastProgressTimer.setSingleShot(true);
    connect(&m_imgFastProgressTimer, &QTimer::timeout, this, &BleManager::imgUpdateFastProgress);

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
    m_rssiRefreshTimer.start();
}

void BleManager::stopScan()
{
    m_scanTimer.stop();
    m_rssiRefreshTimer.stop();
    if (m_discoveryAgent->isActive())
        m_discoveryAgent->stop();

    if (m_connState == Scanning) {
        setConnState(Disconnected);
        setDebugInfo(tr("Scan finished, found %1 device(s)").arg(m_scannedDevices.size()));
    }
}

void BleManager::restartScanForRssi()
{
    if (m_connState != Scanning || !m_discoveryAgent)
        return;
    // 短暂停止后立即重启，不清除设备列表
    m_discoveryAgent->stop();
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
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
    // RSSI 刷新重启会触发 finished 信号，忽略（scanTimer 仍在运行说明还没到时间）
    if (m_connState == Scanning && m_rssiRefreshTimer.isActive())
        return;
    if (m_connState == Scanning) {
        m_rssiRefreshTimer.stop();
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
    // 停止图片传输
    m_imgCancelled = true;
    m_imgAckTimer.stop();
    m_imgResultTimer.stop();
    m_imgFastPaceTimer.stop();
    m_imgFastProgressTimer.stop();
    ImageSendState idle;
    idle.type = ImgIdle;
    setImageSendState(idle);
    imgCleanup();

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

    m_writeQueue.clear();
    m_writePending = false;

    m_topoQueryTimer.stop();
    m_topoQuerying = false;
    emit topoQueryingChanged(false);

    m_cccdReady = false;
    emit cccdReadyChanged(false);

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
    // 清理图片传输
    m_imgCancelled = true;
    m_imgAckTimer.stop();
    m_imgResultTimer.stop();
    m_imgFastPaceTimer.stop();
    m_imgFastProgressTimer.stop();
    if (m_imageSendState.type != ImgIdle) {
        ImageSendState done;
        done.type = ImgDone;
        done.success = false;
        done.msg = tr("Connection lost");
        setImageSendState(done);
    }
    imgCleanup();

    m_writeQueue.clear();
    m_writePending = false;

    m_topoQueryTimer.stop();
    m_topoQuerying = false;
    emit topoQueryingChanged(false);

    m_cccdReady = false;
    emit cccdReadyChanged(false);

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
    connect(m_service, &QLowEnergyService::characteristicWritten,
            this, &BleManager::onCharacteristicWritten);
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
        m_cccdReady = true;
        emit cccdReadyChanged(true);
        setDebugInfo(tr("CCCD subscription successful"));
    }
}

void BleManager::onCharacteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    Q_UNUSED(c)
    Q_UNUSED(value)
    m_writePending = false;
    drainWriteQueue();
    // 图片 FAST 模式：写入完成立即发下包，不等 5ms 定时器，速度取决于 BLE 链路 RTT
    if (!m_writePending && !m_imgCancelled
        && m_imageSendState.type == ImgSending
        && m_imgFastSeq >= 0 && m_imgFastSeq <= m_imgTotalPkts) {
        imgSendNextFastPaced();
    }
}

void BleManager::handleNotify(const QByteArray &data)
{
    UpstreamMessage msg = MeshProtocol::parseNotification(data);
    if (msg.type == UpstreamMessage::Unknown) {
        msg.type    = UpstreamMessage::DataFromNode;
        msg.srcAddr = 0;
        msg.payload = data;
    }

    // 处理拓扑响应 → 停止查询状态
    if (msg.type == UpstreamMessage::Topology) {
        m_topoQueryTimer.stop();
        m_topoQuerying = false;
        emit topoQueryingChanged(false);
    }

    // 处理图片相关消息
    if (msg.type == UpstreamMessage::ImageAck)           handleImageAck(msg);
    if (msg.type == UpstreamMessage::ImageResult)        handleImageResult(msg);
    if (msg.type == UpstreamMessage::ImageProgress)      handleImageProgress(msg);
    if (msg.type == UpstreamMessage::ImageMissing)       handleImageMissing(msg);
    if (msg.type == UpstreamMessage::MulticastProgress)  handleMulticastProgress(msg);

    emit messageReceived(msg);
}

// ─── BLE GATT 串行写入队列 ──────────────────────────────

bool BleManager::sendRaw(const QByteArray &data)
{
    if (!m_service || !m_rxChar.isValid()) return false;
    m_writeQueue.enqueue(data);
    drainWriteQueue();
    return true;
}

void BleManager::drainWriteQueue()
{
    if (m_writePending) return;
    if (m_writeQueue.isEmpty()) return;
    QByteArray data = m_writeQueue.dequeue();
    m_writePending = true;
    m_service->writeCharacteristic(m_rxChar, data, QLowEnergyService::WriteWithResponse);
}

// ─── 发送 ───────────────────────────────────────────────

bool BleManager::sendToNode(quint16 dstAddr, const QByteArray &payload)
{
    if (isImageBusy()) return false;
    return sendRaw(MeshProtocol::buildUnicast(dstAddr, payload));
}

bool BleManager::broadcast(const QByteArray &payload)
{
    if (isImageBusy()) return false;
    return sendRaw(MeshProtocol::buildBroadcast(payload));
}

bool BleManager::queryTopology()
{
    if (m_topoQuerying) return false;
    if (isImageBusy()) return false;
    bool sent = sendRaw(MeshProtocol::buildTopoQuery());
    if (sent) {
        m_topoQuerying = true;
        emit topoQueryingChanged(true);
        m_topoQueryTimer.start();
    }
    return sent;
}

// ─── 辅助 ───────────────────────────────────────────────

void BleManager::setImageSendState(const ImageSendState &state)
{
    m_imageSendState = state;
    emit imageSendStateChanged(state);
}

bool BleManager::isImageBusy() const
{
    auto t = m_imageSendState.type;
    return t == ImgSending || t == ImgWaitingAck || t == ImgFinishing
        || t == ImgMeshTransfer || t == ImgMulticastTransfer;
}

void BleManager::imgCleanup()
{
    m_imgData.clear();
    m_imgPackets.clear();
    m_imgMulticastTargets.clear();
}

// ════════════════════════════════════════════════════════
//  图片发送引擎
// ════════════════════════════════════════════════════════

bool BleManager::sendImage(quint16 dstAddr, const QByteArray &data, int width, int height,
                            ImageSendMode mode, quint8 imageMode)
{
    if (isImageBusy()) return false;
    if (!m_service || !m_rxChar.isValid()) return false;

    m_imgDstAddr = dstAddr;
    m_imgData    = data;
    m_imgWidth   = width;
    m_imgHeight  = height;
    m_imgMode    = mode;
    m_imgCancelled = false;
    m_imgNextSeq = 0;
    m_imgMulticastTargets.clear();

    // 分包
    m_imgPackets.clear();
    for (int i = 0; i < data.size(); i += MeshProtocol::IMG_PKT_PAYLOAD) {
        m_imgPackets.append(data.mid(i, MeshProtocol::IMG_PKT_PAYLOAD));
    }
    m_imgTotalPkts = m_imgPackets.size();

    qDebug() << "[BLE] sendImage: dst=0x" << MeshProtocol::addrToHex4(dstAddr)
             << width << "x" << height << "data=" << data.size() << "B pkts=" << m_imgTotalPkts
             << "mode=" << (mode == FastMode ? "FAST" : "ACK");

    // 1) 发送 START 帧
    quint8 xfer = (mode == AckMode) ? MeshProtocol::IMG_XFER_ACK : MeshProtocol::IMG_XFER_FAST;
    sendRaw(MeshProtocol::buildImageStart(dstAddr, static_cast<quint16>(data.size()),
                                           static_cast<quint16>(m_imgTotalPkts),
                                           static_cast<quint16>(width), static_cast<quint16>(height),
                                           imageMode, xfer));

    // 2) 开始发送数据分包
    ImageSendState state;
    state.type  = ImgSending;
    state.seq   = 0;
    state.total = m_imgTotalPkts;
    state.mode  = mode;
    setImageSendState(state);

    if (mode == FastMode) {
        imgSendAllPacketsFast();
    } else {
        imgSendNextPacketAck();
    }
    return true;
}

bool BleManager::sendImageMulticast(const QList<quint16> &targets, const QByteArray &data,
                                     int width, int height, quint8 imageMode)
{
    if (targets.isEmpty() || targets.size() > 8) return false;
    if (isImageBusy()) return false;
    if (!m_service || !m_rxChar.isValid()) return false;

    m_imgDstAddr = 0xFFFF;  // 组播数据包使用广播地址
    m_imgData    = data;
    m_imgWidth   = width;
    m_imgHeight  = height;
    m_imgMode    = FastMode;
    m_imgCancelled = false;
    m_imgNextSeq = 0;
    m_imgMulticastTargets = targets;

    m_imgPackets.clear();
    for (int i = 0; i < data.size(); i += MeshProtocol::IMG_PKT_PAYLOAD) {
        m_imgPackets.append(data.mid(i, MeshProtocol::IMG_PKT_PAYLOAD));
    }
    m_imgTotalPkts = m_imgPackets.size();

    qDebug() << "[BLE] sendImageMulticast: targets=" << targets.size()
             << width << "x" << height << "data=" << data.size() << "B pkts=" << m_imgTotalPkts;

    // 1) 发送 MCAST_START
    sendRaw(MeshProtocol::buildImageMulticastStart(targets, static_cast<quint16>(data.size()),
                                                    static_cast<quint16>(m_imgTotalPkts),
                                                    static_cast<quint16>(width),
                                                    static_cast<quint16>(height), imageMode));

    // 2) 开始发送数据分包 (FAST 模式)
    ImageSendState state;
    state.type  = ImgSending;
    state.seq   = 0;
    state.total = m_imgTotalPkts;
    state.mode  = FastMode;
    setImageSendState(state);

    imgSendAllPacketsFast();
    return true;
}

void BleManager::cancelImageSend()
{
    if (m_imgCancelled) return;
    m_imgCancelled = true;
    m_imgAckTimer.stop();
    m_imgResultTimer.stop();
    m_imgFastPaceTimer.stop();
    m_imgFastProgressTimer.stop();
    m_writeQueue.clear();
    if (m_imgDstAddr != 0) sendRaw(MeshProtocol::buildImageCancel(m_imgDstAddr));

    ImageSendState state;
    state.type = ImgCancelled;
    state.msg  = tr("Cancelled");
    setImageSendState(state);
    setDebugInfo(tr("Image transfer cancelled"));
    imgCleanup();
}

void BleManager::resetImageSendState()
{
    ImageSendState state;
    state.type = ImgIdle;
    setImageSendState(state);
}

// ─── FAST 模式 ──────────────────────────────────────────

void BleManager::imgSendAllPacketsFast()
{
    m_imgFastSeq = 0;
    imgSendNextFastPaced();
    m_imgFastProgressTimer.start(50);
}

void BleManager::imgSendNextFastPaced()
{
    if (m_imgCancelled) return;

    int seq = m_imgFastSeq;
    if (seq >= m_imgTotalPkts) {
        // 所有数据包发完, 发 END 帧
        quint16 crc = MeshProtocol::crc16(m_imgData);
        sendRaw(MeshProtocol::buildImageEnd(m_imgDstAddr, crc));
        m_imgFastSeq = m_imgTotalPkts + 1;  // 防止回调重复发 END
        qDebug() << "[BLE] imgFastPaced: all" << m_imgTotalPkts << "pkts + END sent";
        return;
    }

    QByteArray frame = MeshProtocol::buildImageData(m_imgDstAddr, static_cast<quint16>(seq), m_imgPackets[seq]);
    sendRaw(frame);
    m_imgFastSeq = seq + 1;
    // 不再使用定时器延迟，由 onCharacteristicWritten 回调驱动
}

void BleManager::imgUpdateFastProgress()
{
    if (m_imgCancelled) return;
    auto t = m_imageSendState.type;
    if (t != ImgSending && t != ImgMeshTransfer && t != ImgFinishing && t != ImgMulticastTransfer) return;

    int sent = qBound(0, m_imgFastSeq, m_imgTotalPkts);

    if (sent >= m_imgTotalPkts) {
        // BLE 上传完成 → 等待网关流控/组播
        if (t == ImgSending) {
            if (!m_imgMulticastTargets.isEmpty()) {
                ImageSendState state;
                state.type = ImgMulticastTransfer;
                state.mcastCompleted = 0;
                state.mcastTotalTargets = m_imgMulticastTargets.size();
                setImageSendState(state);
                setDebugInfo(tr("Upload complete, waiting for multicast..."));
            } else {
                ImageSendState state;
                state.type = ImgMeshTransfer;
                state.rxCount = 0;
                state.total = 0;
                state.phase = 0;
                setImageSendState(state);
                setDebugInfo(tr("Upload complete, waiting for gateway..."));
            }
        }
        // 动态超时 = 30s + 0.5s × 包数 + 组播额外加时
        int timeoutMs = 30000 + m_imgTotalPkts * 500 + m_imgMulticastTargets.size() * 10000;
        m_imgResultTimer.stop();
        m_imgResultTimer.start(timeoutMs);
    } else {
        // 仅在 Sending 状态时更新上传进度
        if (t == ImgSending) {
            ImageSendState state;
            state.type  = ImgSending;
            state.seq   = sent;
            state.total = m_imgTotalPkts;
            state.mode  = FastMode;
            setImageSendState(state);
            setDebugInfo(tr("Uploading to gateway %1/%2").arg(sent).arg(m_imgTotalPkts));
        }
        m_imgFastProgressTimer.start(50);
    }
}

// ─── ACK 模式 ───────────────────────────────────────────

void BleManager::imgSendNextPacketAck()
{
    if (m_imgCancelled) return;

    int seq = m_imgNextSeq;
    if (seq >= m_imgTotalPkts) {
        quint16 crc = MeshProtocol::crc16(m_imgData);
        sendRaw(MeshProtocol::buildImageEnd(m_imgDstAddr, crc));
        ImageSendState state;
        state.type = ImgFinishing;
        setImageSendState(state);
        setDebugInfo(tr("Image data sent, waiting for confirmation..."));
        m_imgResultTimer.stop();
        m_imgResultTimer.start(15000);
        return;
    }

    QByteArray frame = MeshProtocol::buildImageData(m_imgDstAddr, static_cast<quint16>(seq), m_imgPackets[seq]);
    sendRaw(frame);
    ImageSendState state;
    state.type  = ImgWaitingAck;
    state.seq   = seq;
    state.total = m_imgTotalPkts;
    setImageSendState(state);
    setDebugInfo(tr("Sending image %1/%2 (waiting ACK)").arg(seq + 1).arg(m_imgTotalPkts));
    m_imgAckTimer.stop();
    m_imgAckTimer.start(3000);
}

// ─── 图片消息处理 ────────────────────────────────────────

void BleManager::handleImageAck(const UpstreamMessage &msg)
{
    auto t = m_imageSendState.type;
    if (t != ImgSending && t != ImgFinishing && t != ImgMeshTransfer && t != ImgWaitingAck) return;

    if (m_imgMode != AckMode) {
        // 快速模式: ACK 不驱动发包, 但重置超时
        if (t == ImgFinishing || t == ImgMeshTransfer) {
            m_imgResultTimer.stop();
            int timeoutMs = 30000 + m_imgTotalPkts * 500;
            m_imgResultTimer.start(timeoutMs);
        }
        return;
    }
    if (m_imgCancelled) return;

    m_imgAckTimer.stop();
    switch (msg.ackStatus) {
    case MeshProtocol::IMG_ACK_OK: {
        m_imgNextSeq = msg.ackSeq + 1;
        ImageSendState state;
        state.type  = ImgSending;
        state.seq   = m_imgNextSeq;
        state.total = m_imgTotalPkts;
        state.mode  = AckMode;
        setImageSendState(state);
        imgSendNextPacketAck();
        break;
    }
    case MeshProtocol::IMG_ACK_RESEND:
        m_imgNextSeq = msg.ackSeq;
        imgSendNextPacketAck();
        break;
    case MeshProtocol::IMG_ACK_DONE: {
        quint16 crc = MeshProtocol::crc16(m_imgData);
        sendRaw(MeshProtocol::buildImageEnd(m_imgDstAddr, crc));
        ImageSendState state;
        state.type = ImgFinishing;
        setImageSendState(state);
        m_imgResultTimer.stop();
        m_imgResultTimer.start(15000);
        break;
    }
    }
}

void BleManager::handleImageResult(const UpstreamMessage &msg)
{
    auto t = m_imageSendState.type;
    if (t != ImgSending && t != ImgFinishing && t != ImgMeshTransfer && t != ImgWaitingAck) return;

    m_imgAckTimer.stop();
    m_imgResultTimer.stop();
    m_imgFastProgressTimer.stop();
    m_imgFastPaceTimer.stop();

    QString resultMsg = MeshProtocol::imageResultToString(msg.imgResultStatus);
    bool success = (msg.imgResultStatus == MeshProtocol::IMG_RESULT_OK);

    ImageSendState state;
    state.type    = ImgDone;
    state.success = success;
    state.msg     = resultMsg;
    setImageSendState(state);
    setDebugInfo(resultMsg);
    imgCleanup();
}

void BleManager::handleImageProgress(const UpstreamMessage &msg)
{
    auto t = m_imageSendState.type;
    if (t != ImgMeshTransfer && t != ImgFinishing && t != ImgSending) return;

    ImageSendState state;
    state.type    = ImgMeshTransfer;
    state.rxCount = msg.progressRxCount;
    state.total   = msg.progressTotal;
    state.phase   = msg.progressPhase;
    setImageSendState(state);

    QString phaseText = (msg.progressPhase == 0) ? tr("transferring") : tr("retransmitting");
    setDebugInfo(tr("Mesh %1: %2/%3").arg(phaseText).arg(msg.progressRxCount).arg(msg.progressTotal));

    // 重置超时
    m_imgResultTimer.stop();
    int timeoutMs = 30000 + m_imgTotalPkts * 500;
    m_imgResultTimer.start(timeoutMs);
}

void BleManager::handleImageMissing(const UpstreamMessage &msg)
{
    setDebugInfo(tr("Gateway retransmitting (%1 packets missing)...").arg(msg.totalMissing));
    // 重置超时
    m_imgResultTimer.stop();
    int timeoutMs = 30000 + m_imgTotalPkts * 500;
    m_imgResultTimer.start(timeoutMs);
}

void BleManager::handleMulticastProgress(const UpstreamMessage &msg)
{
    auto t = m_imageSendState.type;
    if (t != ImgMulticastTransfer && t != ImgSending) return;

    QMap<quint16, quint8> newResults;
    if (t == ImgMulticastTransfer)
        newResults = m_imageSendState.mcastResults;
    newResults[msg.mcastLatestAddr] = msg.mcastLatestStatus;

    if (msg.mcastCompleted >= msg.mcastTotalTargets) {
        // 所有目标完成
        ImageSendState state;
        state.type = ImgMulticastDone;
        state.mcastResults = newResults;
        setImageSendState(state);

        int successCount = 0;
        for (auto it = newResults.begin(); it != newResults.end(); ++it)
            if (it.value() == 0) ++successCount;
        setDebugInfo(tr("Multicast done: %1/%2 succeeded").arg(successCount).arg(newResults.size()));

        m_imgResultTimer.stop();
        m_imgFastProgressTimer.stop();
        imgCleanup();
    } else {
        ImageSendState state;
        state.type = ImgMulticastTransfer;
        state.mcastCompleted = msg.mcastCompleted;
        state.mcastTotalTargets = msg.mcastTotalTargets;
        state.mcastResults = newResults;
        setImageSendState(state);

        QString statusText;
        switch (msg.mcastLatestStatus) {
        case 0: statusText = tr("OK"); break;
        case 1: statusText = tr("OOM"); break;
        case 2: statusText = tr("Timeout"); break;
        case 4: statusText = tr("CRC"); break;
        default: statusText = tr("Fail"); break;
        }
        setDebugInfo(tr("Multicast %1/%2 [0x%3=%4]")
                     .arg(msg.mcastCompleted).arg(msg.mcastTotalTargets)
                     .arg(MeshProtocol::addrToHex4(msg.mcastLatestAddr))
                     .arg(statusText));

        // 重置超时
        m_imgResultTimer.stop();
        int timeoutMs = 30000 + m_imgTotalPkts * 500 + m_imgMulticastTargets.size() * 10000;
        m_imgResultTimer.start(timeoutMs);
    }
}

void BleManager::onImageAckTimeout()
{
    if (m_imgCancelled) return;
    int retrySeq = m_imgNextSeq;
    if (retrySeq < m_imgTotalPkts) {
        setDebugInfo(tr("ACK timeout, resending %1/%2").arg(retrySeq + 1).arg(m_imgTotalPkts));
        imgSendNextPacketAck();
    } else {
        ImageSendState state;
        state.type = ImgDone;
        state.success = false;
        state.msg = tr("ACK timeout, transfer failed");
        setImageSendState(state);
        imgCleanup();
    }
}

void BleManager::onImageResultTimeout()
{
    auto t = m_imageSendState.type;
    if (t == ImgFinishing || t == ImgMeshTransfer) {
        ImageSendState state;
        state.type = ImgDone;
        state.success = false;
        state.msg = tr("No confirmation from target (gateway transfer may have timed out)");
        setImageSendState(state);
        setDebugInfo(tr("Transfer timeout, please check network and retry"));
        imgCleanup();
    } else if (t == ImgMulticastTransfer) {
        ImageSendState state;
        state.type = ImgMulticastDone;
        state.mcastResults = m_imageSendState.mcastResults;
        setImageSendState(state);
        setDebugInfo(tr("Multicast timeout: %1/%2 completed")
                     .arg(m_imageSendState.mcastCompleted).arg(m_imageSendState.mcastTotalTargets));
        imgCleanup();
    }
}
