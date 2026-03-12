#ifndef BLEMANAGER_H
#define BLEMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QQueue>
#include <QMap>
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
 * 封装 BLE 扫描、连接、GATT 读写、Notify 订阅、图片发送引擎
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

    /** 图片发送模式 */
    enum ImageSendMode { FastMode, AckMode };
    Q_ENUM(ImageSendMode)

    /** 图片发送状态 (UI 观察用) */
    enum ImageSendStateType {
        ImgIdle,
        ImgSending,         // BLE 上传中
        ImgWaitingAck,      // 等待 ACK
        ImgMeshTransfer,    // 网关流控传输中
        ImgFinishing,       // 数据发送完毕,等待确认
        ImgDone,            // 传输完成
        ImgCancelled,       // 已取消
        ImgMulticastTransfer, // 组播传输中
        ImgMulticastDone      // 组播完成
    };
    Q_ENUM(ImageSendStateType)

    struct ImageSendState {
        ImageSendStateType type = ImgIdle;
        int seq = 0;
        int total = 0;
        ImageSendMode mode = FastMode;
        // MeshTransfer
        int rxCount = 0;
        int phase = 0;
        // Done
        bool success = false;
        QString msg;
        // MulticastTransfer / MulticastDone
        int mcastCompleted = 0;
        int mcastTotalTargets = 0;
        QMap<quint16, quint8> mcastResults;  // addr → status
    };

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
    bool cccdReady() const { return m_cccdReady; }
    bool topoQuerying() const { return m_topoQuerying; }
    ImageSendState imageSendState() const { return m_imageSendState; }
    bool isImageBusy() const;

public slots:
    void startScan();
    void stopScan();
    void connectToDevice(int index);
    void disconnectDevice();
    void updateDeviceName(const QString &name);

    bool sendToNode(quint16 dstAddr, const QByteArray &payload);
    bool broadcast(const QByteArray &payload);
    bool queryTopology();

    // 图片发送
    bool sendImage(quint16 dstAddr, const QByteArray &data, int width, int height,
                   ImageSendMode mode = FastMode, quint8 imageMode = MeshProtocol::IMG_MODE_H_LSB);
    bool sendImageMulticast(const QList<quint16> &targets, const QByteArray &data,
                            int width, int height, quint8 imageMode = MeshProtocol::IMG_MODE_H_LSB);
    void cancelImageSend();
    void resetImageSendState();

signals:
    void connStateChanged(BleManager::ConnState state);
    void scannedDevicesChanged();
    void deviceNameChanged(const QString &name);
    void debugInfoChanged(const QString &info);
    void messageReceived(const UpstreamMessage &msg);
    void cccdReadyChanged(bool ready);
    void topoQueryingChanged(bool querying);
    void imageSendStateChanged(const BleManager::ImageSendState &state);

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
    void onCharacteristicWritten(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void onDescriptorWritten(const QLowEnergyDescriptor &d, const QByteArray &value);

private:
    void setConnState(ConnState s);
    void setDebugInfo(const QString &info);
    void setImageSendState(const ImageSendState &state);
    void doStartScan();
    void restartScanForRssi();
    void handleNotify(const QByteArray &data);
    void setupService();

    // BLE GATT 串行写入队列
    bool sendRaw(const QByteArray &data);
    void drainWriteQueue();

    // 图片发送引擎
    void imgSendAllPacketsFast();
    void imgSendNextFastPaced();
    void imgUpdateFastProgress();
    void imgSendNextPacketAck();
    void handleImageAck(const UpstreamMessage &msg);
    void handleImageResult(const UpstreamMessage &msg);
    void handleImageProgress(const UpstreamMessage &msg);
    void handleImageMissing(const UpstreamMessage &msg);
    void handleMulticastProgress(const UpstreamMessage &msg);
    void onImageAckTimeout();
    void onImageResultTimeout();
    void imgCleanup();

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
    bool                            m_cccdReady = false;
    bool                            m_topoQuerying = false;

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QLowEnergyController           *m_controller     = nullptr;
    QLowEnergyService              *m_service        = nullptr;
    QLowEnergyCharacteristic        m_rxChar;         // Write characteristic
    QLowEnergyCharacteristic        m_txChar;         // Notify characteristic
    QTimer                          m_scanTimer;
    QTimer                          m_rssiRefreshTimer;
    QTimer                          m_topoQueryTimer;

    // BLE MTU
    int                             m_negotiatedMtu = 23;

    // BLE 写入队列
    QQueue<QByteArray>              m_writeQueue;
    bool                            m_writePending = false;

    // 图片发送引擎
    ImageSendState                  m_imageSendState;
    quint16                         m_imgDstAddr = 0;
    QByteArray                      m_imgData;
    QList<QByteArray>               m_imgPackets;
    int                             m_imgTotalPkts = 0;
    int                             m_imgNextSeq = 0;
    ImageSendMode                   m_imgMode = FastMode;
    bool                            m_imgCancelled = false;
    bool                            m_imgFastWriteNoResp = false; // FAST模式使用WriteWithoutResponse
    int                             m_imgWidth = 0;
    int                             m_imgHeight = 0;
    int                             m_imgFastSeq = 0;
    QList<quint16>                  m_imgMulticastTargets;
    QTimer                          m_imgAckTimer;
    QTimer                          m_imgResultTimer;
    QTimer                          m_imgFastPaceTimer;
    QTimer                          m_imgFastProgressTimer;
};

#endif // BLEMANAGER_H
