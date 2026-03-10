#ifndef SOCKETTRANSPORT_H
#define SOCKETTRANSPORT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>
#include <QString>
#include <QList>
#include "meshprotocol.h"

/**
 * WiFi Socket 图传客户端
 * 通过 TCP 直连嵌入式设备 SoftAP (192.168.43.1:8080) 发送图片
 *   协议格式:
 *   [Header 14B] magic(0xAA55) + width(2) + height(2) + mode(1) + dst_hi(1) + dst_lo(1) + reserved(1) + data_size(4)
 *   [Data N bytes] 图片原始数据  (dst=0xFFFF: 本地显示; 其他: Mesh转发)
 *   设备回复: 1 byte (0=OK, 1=OOM, 2=fail, 3=BUSY)
 */
/** WiFi 发现的设备信息 */
struct WifiDevice {
    QString name;   // 显示名称，例如 "NearLink_EPaper"
    QString host;   // IP 地址
    quint16 port;   // TCP 端口
};

class SocketTransport : public QObject
{
    Q_OBJECT

public:
    enum State { Idle, Connecting, Sending, WaitingReply, Done, Error };
    Q_ENUM(State)

    explicit SocketTransport(QObject *parent = nullptr);
    ~SocketTransport() override;

    /** 设备 IP 和端口 */
    void setHost(const QString &ip, quint16 port = 8080);

    /** 发送图片数据
     *  dstAddr=0xFFFF: 网关本地显示; 其他: Mesh转发到指定节点 */
    bool sendImage(const QByteArray &data, int width, int height, quint8 mode,
                   quint16 dstAddr = 0xFFFF);

    /** 异步查询 WiFi 网关下的 Mesh 节点拓扑
     *  结果通过 wifiTopologyReceived 信号返回 */
    void queryTopologyWifi();

    /** 当前状态 */
    State state() const { return m_state; }
    bool isBusy() const { return m_state != Idle && m_state != Done && m_state != Error; }

    /** 取消当前传输 */
    void cancel();

    /**
     * 异步探测 WiFi 设备是否在线
     * 向 host:port 发起 TCP 连接，成功则 emit wifiDeviceFound
     */
    void startProbe(const QString &host = QStringLiteral("192.168.43.1"),
                    quint16 port = 8080,
                    const QString &name = QStringLiteral("NearLink_EPaper"));
    void stopProbe();

signals:
    void stateChanged(SocketTransport::State state);
    void progressChanged(int bytesSent, int totalBytes);
    void finished(bool success, const QString &message);
    /** 探测到 WiFi 设备在线 */
    void wifiDeviceFound(const WifiDevice &device);
    /** 探测完成（无论是否找到） */
    void probeFinished();
    /** WiFi TOPO 查询完成，返回节点列表 */
    void wifiTopologyReceived(const QList<MeshNode> &nodes);

private slots:
    void onConnected();
    void onBytesWritten(qint64 bytes);
    void onReadyRead();
    void onError(QAbstractSocket::SocketError err);
    void onTimeout();
    void onProbeConnected();
    void onProbeReadyRead();
    void onProbeError();
    void onProbeTimeout();
    void onTopoConnected();
    void onTopoReadyRead();
    void onTopoError(QAbstractSocket::SocketError err);
    void onTopoTimeout();

private:
    void setState(State s);
    void cleanup();

    QTcpSocket *m_socket = nullptr;
    QTimer      m_timeout;
    State       m_state = Idle;
    QString     m_host = QStringLiteral("192.168.43.1");
    quint16     m_port = 8080;

    QByteArray  m_sendBuf;
    int         m_totalBytes = 0;
    int         m_sentBytes = 0;

    // 探测相关
    QTcpSocket *m_probeSocket = nullptr;
    QTimer      m_probeTimeout;
    WifiDevice  m_probeDevice;

    // TOPO 查询相关
    QTcpSocket *m_topoSocket  = nullptr;
    QTimer      m_topoTimeout;
    QByteArray  m_topoBuf;
};

#endif // SOCKETTRANSPORT_H
