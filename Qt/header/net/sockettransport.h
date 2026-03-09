#ifndef SOCKETTRANSPORT_H
#define SOCKETTRANSPORT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QTimer>

/**
 * WiFi Socket 图传客户端
 * 通过 TCP 直连嵌入式设备 SoftAP (192.168.43.1:8080) 发送图片
 *
 * 协议格式:
 *   [Header 12B] magic(0xAA55) + width(2) + height(2) + mode(1) + reserved(1) + data_size(4)
 *   [Data N bytes] 图片原始数据
 *   设备回复: 1 byte (0=OK, 1=OOM, 2=fail)
 */
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

    /** 发送图片数据 */
    bool sendImage(const QByteArray &data, int width, int height, quint8 mode);

    /** 当前状态 */
    State state() const { return m_state; }
    bool isBusy() const { return m_state != Idle && m_state != Done && m_state != Error; }

    /** 取消当前传输 */
    void cancel();

signals:
    void stateChanged(SocketTransport::State state);
    void progressChanged(int bytesSent, int totalBytes);
    void finished(bool success, const QString &message);

private slots:
    void onConnected();
    void onBytesWritten(qint64 bytes);
    void onReadyRead();
    void onError(QAbstractSocket::SocketError err);
    void onTimeout();

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
};

#endif // SOCKETTRANSPORT_H
