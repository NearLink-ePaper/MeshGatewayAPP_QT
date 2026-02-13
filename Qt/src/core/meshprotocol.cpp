#include "meshprotocol.h"

QByteArray MeshProtocol::buildUnicast(quint16 dstAddr, const QByteArray &data)
{
    QByteArray frame;
    frame.reserve(5 + data.size());
    frame.append(static_cast<char>(MAGIC));
    frame.append(static_cast<char>(0x01));                       // CMD_UNICAST
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
    frame.append(static_cast<char>(0x02));   // CMD_BROADCAST
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
    frame.append(static_cast<char>(0x03));   // CMD_TOPO_QUERY
    return frame;
}

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
    case 0x81: return parseDataMessage(data);
    case 0x83: return parseTopoResponse(data);
    default:   return msg;
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

    msg.type    = UpstreamMessage::DataFromNode;
    msg.srcAddr = srcAddr;
    msg.payload = data.mid(5, payloadLen);
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
