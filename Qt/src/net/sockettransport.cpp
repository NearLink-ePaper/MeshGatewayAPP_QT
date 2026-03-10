#include "sockettransport.h"
#include <QDataStream>
#include <QDebug>

static constexpr quint8  MAGIC_0      = 0xAA;
static constexpr quint8  MAGIC_1      = 0x55;
static constexpr int     HEADER_SIZE  = 14;    /* 14B: magic(2)+w(2)+h(2)+mode(1)+dst(2)+rsv(1)+size(4) */
static constexpr int     TIMEOUT_MS   = 15000;
static constexpr int     TOPO_TIMEOUT = 5500;  /* TOPO: 3.5s 固件收集 + 余量 */
static constexpr quint8  CMD_MAGIC    = 0xFD;
static constexpr quint8  CMD_TOPO     = 0x03;

SocketTransport::SocketTransport(QObject *parent)
    : QObject(parent)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, &SocketTransport::onTimeout);
}

SocketTransport::~SocketTransport()
{
    cleanup();
}

void SocketTransport::setHost(const QString &ip, quint16 port)
{
    m_host = ip;
    m_port = port;
}

bool SocketTransport::sendImage(const QByteArray &data, int width, int height,
                                quint8 mode, quint16 dstAddr)
{
    if (isBusy()) return false;

    /* 构建 14 字节协议头: magic(2)+w(2)+h(2)+mode(1)+dst_hi(1)+dst_lo(1)+rsv(1)+size(4) */
    QByteArray header(HEADER_SIZE, '\0');
    header[0]  = static_cast<char>(MAGIC_0);
    header[1]  = static_cast<char>(MAGIC_1);
    header[2]  = static_cast<char>((width  >> 8) & 0xFF);
    header[3]  = static_cast<char>(width  & 0xFF);
    header[4]  = static_cast<char>((height >> 8) & 0xFF);
    header[5]  = static_cast<char>(height & 0xFF);
    header[6]  = static_cast<char>(mode);
    header[7]  = static_cast<char>((dstAddr >> 8) & 0xFF); /* dst_hi */
    header[8]  = static_cast<char>(dstAddr & 0xFF);         /* dst_lo */
    header[9]  = 0; /* reserved */
    quint32 sz = static_cast<quint32>(data.size());
    header[10] = static_cast<char>((sz >> 24) & 0xFF);
    header[11] = static_cast<char>((sz >> 16) & 0xFF);
    header[12] = static_cast<char>((sz >> 8)  & 0xFF);
    header[13] = static_cast<char>(sz & 0xFF);

    m_sendBuf = header + data;
    m_totalBytes = m_sendBuf.size();
    m_sentBytes = 0;

    /* 创建新 socket */
    cleanup();
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &SocketTransport::onConnected);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &SocketTransport::onBytesWritten);
    connect(m_socket, &QTcpSocket::readyRead, this, &SocketTransport::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &SocketTransport::onError);

    setState(Connecting);
    m_timeout.start(TIMEOUT_MS);
    m_socket->connectToHost(m_host, m_port);

    qDebug() << "[WiFi] connecting to" << m_host << ":" << m_port
             << "data=" << data.size() << "B";
    return true;
}

void SocketTransport::cancel()
{
    if (m_state == Idle) return;
    cleanup();
    setState(Error);
    emit finished(false, tr("Cancelled"));
}

void SocketTransport::onConnected()
{
    qDebug() << "[WiFi] connected, sending" << m_totalBytes << "bytes";
    setState(Sending);
    m_timeout.start(TIMEOUT_MS);
    m_socket->write(m_sendBuf);
}

void SocketTransport::onBytesWritten(qint64 bytes)
{
    m_sentBytes += static_cast<int>(bytes);
    emit progressChanged(m_sentBytes, m_totalBytes);

    if (m_sentBytes >= m_totalBytes) {
        setState(WaitingReply);
        m_timeout.start(TIMEOUT_MS);
        qDebug() << "[WiFi] all bytes sent, waiting reply";
    }
}

void SocketTransport::onReadyRead()
{
    /* 服务器可能在 Sending 阶段就发错误响应（如 BUSY/OOM）并关闭连接，
     * 需在任何活跃状态下处理，不能只限 WaitingReply */
    if (m_state == Idle || m_state == Done || m_state == Error) return;

    QByteArray resp = m_socket->readAll();
    m_timeout.stop();

    if (resp.isEmpty()) {
        if (m_state != WaitingReply) return;  /* 非等待响应阶段不处理空数据 */
        setState(Error);
        emit finished(false, tr("Empty response"));
        cleanup();
        return;
    }

    quint8 code = static_cast<quint8>(resp[0]);
    bool ok = (code == 0);
    QString msg;
    switch (code) {
    case 0: msg = tr("OK"); break;
    case 1: msg = tr("Device OOM"); break;
    case 2: msg = tr("Decode failed"); break;
    case 3: msg = tr("Device busy (BLE active)"); break;
    default: msg = tr("Unknown error %1").arg(code); break;
    }

    qDebug() << "[WiFi] response:" << code << msg;
    setState(ok ? Done : Error);
    emit finished(ok, msg);
    cleanup();
}

void SocketTransport::onError(QAbstractSocket::SocketError err)
{
    Q_UNUSED(err)
    if (m_state == Idle || m_state == Done || m_state == Error) return;

    QString msg = m_socket ? m_socket->errorString() : tr("Unknown error");
    qDebug() << "[WiFi] socket error:" << msg;
    m_timeout.stop();
    setState(Error);
    emit finished(false, msg);
    cleanup();
}

void SocketTransport::onTimeout()
{
    qDebug() << "[WiFi] timeout in state" << m_state;
    setState(Error);
    emit finished(false, tr("Timeout"));
    cleanup();
}

void SocketTransport::setState(State s)
{
    if (m_state == s) return;
    m_state = s;
    emit stateChanged(s);
}

void SocketTransport::cleanup()
{
    m_timeout.stop();
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_sendBuf.clear();
}

// ─── WiFi 探测 ────────────────────────────────────────────

void SocketTransport::startProbe(const QString &host, quint16 port, const QString &name)
{
    stopProbe();

    m_probeDevice = { name, host, port };

    m_probeSocket = new QTcpSocket(this);
    connect(m_probeSocket, &QTcpSocket::connected,
            this, &SocketTransport::onProbeConnected);
    connect(m_probeSocket, &QTcpSocket::readyRead,
            this, &SocketTransport::onProbeReadyRead);
    connect(m_probeSocket, &QTcpSocket::errorOccurred,
            this, &SocketTransport::onProbeError);

    m_probeTimeout.setSingleShot(true);
    m_probeTimeout.setInterval(3000);
    connect(&m_probeTimeout, &QTimer::timeout,
            this, &SocketTransport::onProbeTimeout, Qt::UniqueConnection);

    m_probeTimeout.start();
    m_probeSocket->connectToHost(host, port);
    qDebug() << "[WiFi probe] connecting to" << host << ":" << port;
}

void SocketTransport::stopProbe()
{
    m_probeTimeout.stop();
    if (m_probeSocket) {
        m_probeSocket->disconnect(this);
        m_probeSocket->abort();
        m_probeSocket->deleteLater();
        m_probeSocket = nullptr;
    }
}

void SocketTransport::onProbeConnected()
{
    /* 发送探测魔数，服务器将回复网关名称 */
    const char probeByte = (char)0xFE;
    m_probeSocket->write(&probeByte, 1);
    qDebug() << "[WiFi probe] connected, sent probe byte";
    /* 等待 onProbeReadyRead 返回网关名称 */
}

void SocketTransport::onProbeReadyRead()
{
    if (!m_probeSocket) return;
    QByteArray data = m_probeSocket->readAll();
    if (data.isEmpty()) return;

    QString name = QString::fromLatin1(data.constData(), data.size()).trimmed();
    if (!name.isEmpty()) {
        m_probeDevice.name = name;  /* 使用服务器返回的实际网关名称 */
    }
    qDebug() << "[WiFi probe] found device:" << m_probeDevice.name;
    m_probeTimeout.stop();
    emit wifiDeviceFound(m_probeDevice);
    emit probeFinished();
    stopProbe();
}

void SocketTransport::onProbeError()
{
    qDebug() << "[WiFi probe] not found:" << m_probeDevice.host;
    m_probeTimeout.stop();
    emit probeFinished();
    stopProbe();
}

void SocketTransport::onProbeTimeout()
{
    qDebug() << "[WiFi probe] timeout";
    emit probeFinished();
    stopProbe();
}

// ─── WiFi TOPO 查询 ──────────────────────────────────────────────────────────

void SocketTransport::queryTopologyWifi()
{
    if (m_topoSocket) {
        m_topoSocket->disconnect(this);
        m_topoSocket->abort();
        m_topoSocket->deleteLater();
        m_topoSocket = nullptr;
    }

    m_topoSocket = new QTcpSocket(this);
    connect(m_topoSocket, &QTcpSocket::connected,     this, &SocketTransport::onTopoConnected);
    connect(m_topoSocket, &QTcpSocket::readyRead,     this, &SocketTransport::onTopoReadyRead);
    connect(m_topoSocket, &QTcpSocket::errorOccurred, this, &SocketTransport::onTopoError);

    m_topoTimeout.setSingleShot(true);
    m_topoTimeout.setInterval(TOPO_TIMEOUT);
    connect(&m_topoTimeout, &QTimer::timeout,
            this, &SocketTransport::onTopoTimeout, Qt::UniqueConnection);

    m_topoBuf.clear();
    m_topoTimeout.start();
    m_topoSocket->connectToHost(m_host, m_port);
    qDebug() << "[WiFi TOPO] connecting to" << m_host << ":" << m_port;
}

void SocketTransport::onTopoConnected()
{
    char cmd[2] = { static_cast<char>(CMD_MAGIC), static_cast<char>(CMD_TOPO) };
    m_topoSocket->write(cmd, 2);
    m_topoSocket->flush();              /* 强制立即发送，不等待 Nagle 定时器 */
    m_topoTimeout.start(TOPO_TIMEOUT);   /* 等待固件 3.5s 收集响应 */
    qDebug() << "[WiFi TOPO] command sent";
}

static QList<MeshNode> parseTopoBuf(const QByteArray &raw)
{
    QList<MeshNode> nodes;
    if (raw.isEmpty()) return nodes;
    int count = static_cast<quint8>(raw[0]);
    for (int i = 0; i < count && (1 + i * 3 + 2) < raw.size(); ++i) {
        quint16 addr = (static_cast<quint8>(raw[1 + i*3]) << 8)
                     |  static_cast<quint8>(raw[2 + i*3]);
        quint8  hops =  static_cast<quint8>(raw[3 + i*3]);
        nodes.append(MeshNode(addr, hops));
    }
    return nodes;
}

void SocketTransport::onTopoReadyRead()
{
    if (!m_topoSocket) return;
    m_topoBuf += m_topoSocket->readAll();
    if (m_topoBuf.isEmpty()) return;

    QList<MeshNode> nodes = parseTopoBuf(m_topoBuf);
    m_topoTimeout.stop();
    qDebug() << "[WiFi TOPO]" << nodes.size() << "nodes received";
    emit wifiTopologyReceived(nodes);

    m_topoSocket->disconnect(this);
    m_topoSocket->abort();
    m_topoSocket->deleteLater();
    m_topoSocket = nullptr;
    m_topoBuf.clear();
}

void SocketTransport::onTopoError(QAbstractSocket::SocketError err)
{
    /* RemoteHostClosedError: 服务器正常发完数据后关闭连接，追加剩余字节再解析 */
    if (err == QAbstractSocket::RemoteHostClosedError && m_topoSocket) {
        m_topoBuf += m_topoSocket->readAll();
        if (!m_topoBuf.isEmpty()) {
            QList<MeshNode> nodes = parseTopoBuf(m_topoBuf);
            m_topoTimeout.stop();
            qDebug() << "[WiFi TOPO] remote closed, parsed" << nodes.size() << "nodes";
            emit wifiTopologyReceived(nodes);
            m_topoSocket->disconnect(this);
            m_topoSocket->abort();
            m_topoSocket->deleteLater();
            m_topoSocket = nullptr;
            m_topoBuf.clear();
            return;
        }
    }
    qDebug() << "[WiFi TOPO] error:" << (m_topoSocket ? m_topoSocket->errorString() : QString());
    m_topoTimeout.stop();
    emit wifiTopologyReceived({});
    if (m_topoSocket) {
        m_topoSocket->disconnect(this);
        m_topoSocket->abort();
        m_topoSocket->deleteLater();
        m_topoSocket = nullptr;
    }
    m_topoBuf.clear();
}

void SocketTransport::onTopoTimeout()
{
    qDebug() << "[WiFi TOPO] timeout";
    emit wifiTopologyReceived({});
    if (m_topoSocket) {
        m_topoSocket->disconnect(this);
        m_topoSocket->abort();
        m_topoSocket->deleteLater();
        m_topoSocket = nullptr;
    }
}
