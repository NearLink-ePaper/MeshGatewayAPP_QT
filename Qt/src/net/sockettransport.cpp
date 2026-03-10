#include "sockettransport.h"
#include <QDataStream>
#include <QDebug>

static constexpr quint8  MAGIC_0 = 0xAA;
static constexpr quint8  MAGIC_1 = 0x55;
static constexpr int     HEADER_SIZE = 12;
static constexpr int     TIMEOUT_MS  = 15000;

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

bool SocketTransport::sendImage(const QByteArray &data, int width, int height, quint8 mode)
{
    if (isBusy()) return false;

    /* 构建协议头 + 数据 */
    QByteArray header(HEADER_SIZE, '\0');
    header[0] = static_cast<char>(MAGIC_0);
    header[1] = static_cast<char>(MAGIC_1);
    header[2] = static_cast<char>((width >> 8) & 0xFF);
    header[3] = static_cast<char>(width & 0xFF);
    header[4] = static_cast<char>((height >> 8) & 0xFF);
    header[5] = static_cast<char>(height & 0xFF);
    header[6] = static_cast<char>(mode);
    header[7] = 0; /* reserved */
    quint32 sz = static_cast<quint32>(data.size());
    header[8]  = static_cast<char>((sz >> 24) & 0xFF);
    header[9]  = static_cast<char>((sz >> 16) & 0xFF);
    header[10] = static_cast<char>((sz >> 8) & 0xFF);
    header[11] = static_cast<char>(sz & 0xFF);

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
