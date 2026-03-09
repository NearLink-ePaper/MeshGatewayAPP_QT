#include "meshprotocol.h"

QByteArray MeshProtocol::buildUnicast(quint16 dstAddr, const QByteArray &data)
{
    QByteArray frame;
    frame.reserve(5 + data.size());
    frame.append(static_cast<char>(MAGIC));
    frame.append(static_cast<char>(CMD_UNICAST));
    frame.append(static_cast<char>((dstAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(dstAddr & 0xFF));
    frame.append(static_cast<char>(data.size() & 0xFF));
    frame.append(data);
    return frame;
}

QByteArray MeshProtocol::buildBroadcast(const QByteArray &data)
{
    QByteArray frame;
    frame.reserve(5 + data.size());
    frame.append(static_cast<char>(MAGIC));
    frame.append(static_cast<char>(CMD_BROADCAST));
    frame.append(static_cast<char>(0xFF));
    frame.append(static_cast<char>(0xFF));
    frame.append(static_cast<char>(data.size() & 0xFF));
    frame.append(data);
    return frame;
}

QByteArray MeshProtocol::buildTopoQuery()
{
    QByteArray frame;
    frame.append(static_cast<char>(MAGIC));
    frame.append(static_cast<char>(CMD_TOPO_QUERY));
    return frame;
}

QByteArray MeshProtocol::buildImageStart(quint16 dstAddr, quint16 totalBytes, quint16 pktCount,
                                          quint16 width, quint16 height,
                                          quint8 mode, quint8 xfer)
{
    QByteArray frame(14, '\0');
    frame[0]  = static_cast<char>(MAGIC);
    frame[1]  = static_cast<char>(CMD_IMG_START);
    frame[2]  = static_cast<char>((dstAddr >> 8) & 0xFF);
    frame[3]  = static_cast<char>(dstAddr & 0xFF);
    frame[4]  = static_cast<char>((totalBytes >> 8) & 0xFF);
    frame[5]  = static_cast<char>(totalBytes & 0xFF);
    frame[6]  = static_cast<char>((pktCount >> 8) & 0xFF);
    frame[7]  = static_cast<char>(pktCount & 0xFF);
    frame[8]  = static_cast<char>((width >> 8) & 0xFF);
    frame[9]  = static_cast<char>(width & 0xFF);
    frame[10] = static_cast<char>((height >> 8) & 0xFF);
    frame[11] = static_cast<char>(height & 0xFF);
    frame[12] = static_cast<char>(mode);
    frame[13] = static_cast<char>(xfer);
    return frame;
}

QByteArray MeshProtocol::buildImageData(quint16 dstAddr, quint16 seq, const QByteArray &data)
{
    QByteArray frame;
    frame.reserve(7 + data.size());
    frame.append(static_cast<char>(MAGIC));
    frame.append(static_cast<char>(CMD_IMG_DATA));
    frame.append(static_cast<char>((dstAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(dstAddr & 0xFF));
    frame.append(static_cast<char>((seq >> 8) & 0xFF));
    frame.append(static_cast<char>(seq & 0xFF));
    frame.append(static_cast<char>(data.size() & 0xFF));
    frame.append(data);
    return frame;
}

QByteArray MeshProtocol::buildImageEnd(quint16 dstAddr, quint16 crc)
{
    QByteArray frame(6, '\0');
    frame[0] = static_cast<char>(MAGIC);
    frame[1] = static_cast<char>(CMD_IMG_END);
    frame[2] = static_cast<char>((dstAddr >> 8) & 0xFF);
    frame[3] = static_cast<char>(dstAddr & 0xFF);
    frame[4] = static_cast<char>((crc >> 8) & 0xFF);
    frame[5] = static_cast<char>(crc & 0xFF);
    return frame;
}

QByteArray MeshProtocol::buildImageCancel(quint16 dstAddr)
{
    QByteArray frame(4, '\0');
    frame[0] = static_cast<char>(MAGIC);
    frame[1] = static_cast<char>(CMD_IMG_CANCEL);
    frame[2] = static_cast<char>((dstAddr >> 8) & 0xFF);
    frame[3] = static_cast<char>(dstAddr & 0xFF);
    return frame;
}

QByteArray MeshProtocol::buildImageMulticastStart(const QList<quint16> &targets,
                                                   quint16 totalBytes, quint16 pktCount,
                                                   quint16 width, quint16 height,
                                                   quint8 mode)
{
    int n = qMin(targets.size(), 8);
    QByteArray frame(2 + 1 + n * 2 + 9, '\0');
    int pos = 0;
    frame[pos++] = static_cast<char>(MAGIC);
    frame[pos++] = static_cast<char>(CMD_IMG_MCAST_START);
    frame[pos++] = static_cast<char>(n);
    for (int i = 0; i < n; ++i) {
        frame[pos++] = static_cast<char>((targets[i] >> 8) & 0xFF);
        frame[pos++] = static_cast<char>(targets[i] & 0xFF);
    }
    frame[pos++] = static_cast<char>((totalBytes >> 8) & 0xFF);
    frame[pos++] = static_cast<char>(totalBytes & 0xFF);
    frame[pos++] = static_cast<char>((pktCount >> 8) & 0xFF);
    frame[pos++] = static_cast<char>(pktCount & 0xFF);
    frame[pos++] = static_cast<char>((width >> 8) & 0xFF);
    frame[pos++] = static_cast<char>(width & 0xFF);
    frame[pos++] = static_cast<char>((height >> 8) & 0xFF);
    frame[pos++] = static_cast<char>(height & 0xFF);
    frame[pos++] = static_cast<char>(mode);
    return frame;
}

// ─── 上行解析 ───────────────────────────────────────────

UpstreamMessage MeshProtocol::parseNotification(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 2)
        return msg;

    quint8 magic = static_cast<quint8>(data.at(0));
    if (magic != MAGIC)
        return msg;

    quint8 cmd = static_cast<quint8>(data.at(1));
    switch (cmd) {
    case CMD_DATA_UP:           return parseDataMessage(data);
    case CMD_TOPO_RESP:         return parseTopoResponse(data);
    case CMD_IMG_ACK:           return parseImageAck(data);
    case CMD_IMG_RESULT:        return parseImageResult(data);
    case CMD_IMG_MISSING:       return parseImageMissing(data);
    case CMD_IMG_PROGRESS:      return parseImageProgress(data);
    case CMD_IMG_MCAST_PROGRESS:return parseMulticastProgress(data);
    default:                    return msg;
    }
}

UpstreamMessage MeshProtocol::parseDataMessage(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 5)
        return msg;

    quint16 srcAddr = (static_cast<quint8>(data.at(2)) << 8) |
                       static_cast<quint8>(data.at(3));
    quint8 payloadLen = static_cast<quint8>(data.at(4));

    if (data.size() < 5 + payloadLen)
        return msg;

    QByteArray payload = data.mid(5, payloadLen);

    // 检查负载中是否嵌套了图片命令
    if (payload.size() >= 2) {
        quint8 innerCmd = static_cast<quint8>(payload.at(0));
        if (innerCmd == CMD_IMG_RESULT && payload.size() >= 2) {
            msg.type = UpstreamMessage::ImageResult;
            msg.srcAddr = srcAddr;
            msg.imgResultStatus = static_cast<quint8>(payload.at(1));
            return msg;
        }
        if (innerCmd == CMD_IMG_ACK && payload.size() >= 4) {
            msg.type = UpstreamMessage::ImageAck;
            msg.srcAddr = srcAddr;
            msg.ackStatus = static_cast<quint8>(payload.at(1));
            msg.ackSeq = (static_cast<quint8>(payload.at(2)) << 8) |
                          static_cast<quint8>(payload.at(3));
            return msg;
        }
    }

    msg.type    = UpstreamMessage::DataFromNode;
    msg.srcAddr = srcAddr;
    msg.payload = payload;
    return msg;
}

UpstreamMessage MeshProtocol::parseTopoResponse(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 5)
        return msg;

    quint16 gwAddr = (static_cast<quint8>(data.at(2)) << 8) |
                      static_cast<quint8>(data.at(3));
    quint8 count   = static_cast<quint8>(data.at(4));

    if (data.size() < 5 + count * 3)
        return msg;

    QList<MeshNode> nodes;
    for (int i = 0; i < count; ++i) {
        int offset = 5 + i * 3;
        quint16 addr = (static_cast<quint8>(data.at(offset)) << 8) |
                        static_cast<quint8>(data.at(offset + 1));
        quint8 hops  = static_cast<quint8>(data.at(offset + 2));
        nodes.append(MeshNode(addr, hops));
    }

    msg.type        = UpstreamMessage::Topology;
    msg.gatewayAddr = gwAddr;
    msg.nodes       = nodes;
    return msg;
}

UpstreamMessage MeshProtocol::parseImageAck(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 7) return msg;
    msg.type      = UpstreamMessage::ImageAck;
    msg.srcAddr   = (static_cast<quint8>(data.at(2)) << 8) | static_cast<quint8>(data.at(3));
    msg.ackStatus = static_cast<quint8>(data.at(4));
    msg.ackSeq    = (static_cast<quint8>(data.at(5)) << 8) | static_cast<quint8>(data.at(6));
    return msg;
}

UpstreamMessage MeshProtocol::parseImageResult(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 5) return msg;
    msg.type            = UpstreamMessage::ImageResult;
    msg.srcAddr         = (static_cast<quint8>(data.at(2)) << 8) | static_cast<quint8>(data.at(3));
    msg.imgResultStatus = static_cast<quint8>(data.at(4));
    return msg;
}

UpstreamMessage MeshProtocol::parseImageMissing(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 6) return msg;
    msg.type         = UpstreamMessage::ImageMissing;
    msg.srcAddr      = (static_cast<quint8>(data.at(2)) << 8) | static_cast<quint8>(data.at(3));
    msg.totalMissing = (static_cast<quint8>(data.at(4)) << 8) | static_cast<quint8>(data.at(5));
    return msg;
}

UpstreamMessage MeshProtocol::parseImageProgress(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 9) return msg;
    msg.type            = UpstreamMessage::ImageProgress;
    msg.srcAddr         = (static_cast<quint8>(data.at(2)) << 8) | static_cast<quint8>(data.at(3));
    msg.progressPhase   = static_cast<quint8>(data.at(4));
    msg.progressRxCount = (static_cast<quint8>(data.at(5)) << 8) | static_cast<quint8>(data.at(6));
    msg.progressTotal   = (static_cast<quint8>(data.at(7)) << 8) | static_cast<quint8>(data.at(8));
    return msg;
}

UpstreamMessage MeshProtocol::parseMulticastProgress(const QByteArray &data)
{
    UpstreamMessage msg;
    if (data.size() < 7) return msg;
    msg.type              = UpstreamMessage::MulticastProgress;
    msg.mcastCompleted    = static_cast<quint8>(data.at(2));
    msg.mcastTotalTargets = static_cast<quint8>(data.at(3));
    msg.mcastLatestAddr   = (static_cast<quint8>(data.at(4)) << 8) | static_cast<quint8>(data.at(5));
    msg.mcastLatestStatus = static_cast<quint8>(data.at(6));
    return msg;
}

// ─── CRC-16/CCITT ──────────────────────────────────────

quint16 MeshProtocol::crc16(const QByteArray &data)
{
    quint16 crc = 0x0000;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= (static_cast<quint8>(data.at(i)) << 8);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
            crc &= 0xFFFF;
        }
    }
    return crc;
}

// ─── 工具函数 ───────────────────────────────────────────

QString MeshProtocol::toHexString(const QByteArray &data)
{
    QStringList parts;
    for (int i = 0; i < data.size(); ++i) {
        parts.append(QString("%1").arg(static_cast<quint8>(data.at(i)), 2, 16, QChar('0')).toUpper());
    }
    return parts.join('-');
}

QString MeshProtocol::addrToHex4(quint16 addr)
{
    return QString("%1").arg(addr, 4, 16, QChar('0')).toUpper();
}

QString MeshProtocol::decodePayloadToStringOrHex(const QByteArray &data)
{
    // 尝试 UTF-8 解码，若全部是可打印字符则返回文本，否则回退 hex
    QString text = QString::fromUtf8(data);
    bool printable = true;
    for (const QChar &ch : text) {
        if (ch.unicode() < 0x20 || ch == QChar(0xFFFD)) {
            printable = false;
            break;
        }
    }
    return printable ? text : toHexString(data);
}

QString MeshProtocol::imageResultToString(quint8 status)
{
    switch (status) {
    case IMG_RESULT_OK:      return QObject::tr("Image received successfully");
    case IMG_RESULT_OOM:     return QObject::tr("Target node out of memory");
    case IMG_RESULT_TIMEOUT: return QObject::tr("Target node receive timeout");
    case IMG_RESULT_CANCEL:  return QObject::tr("Target node cancelled");
    case IMG_RESULT_CRC_ERR: return QObject::tr("CRC check failed (packet loss)");
    default:                 return QObject::tr("Unknown status (%1)").arg(status);
    }
}
