#ifndef MESHPROTOCOL_H
#define MESHPROTOCOL_H

#include <QByteArray>
#include <QString>
#include <QList>
#include <QMetaType>

/**
 * Mesh 网络节点信息
 */
struct MeshNode {
    quint16 addr;   // 节点地址
    quint8  hops;   // 跳数 (0=网关, 1=直连, 2+=多跳)

    MeshNode() : addr(0), hops(0) {}
    MeshNode(quint16 a, quint8 h) : addr(a), hops(h) {}

    bool operator==(const MeshNode &o) const { return addr == o.addr && hops == o.hops; }
};

/**
 * 上行消息类型
 */
struct UpstreamMessage {
    enum Type { Unknown, DataFromNode, Topology };
    Type type = Unknown;

    // DataFromNode 字段
    quint16    srcAddr  = 0;
    QByteArray payload;

    // Topology 字段
    quint16          gatewayAddr = 0;
    QList<MeshNode>  nodes;
};

/**
 * BLE-Mesh 网关通信协议编解码
 *
 * 下行 (手机 → 网关):
 *   AA 01 DST_HI DST_LO LEN PAYLOAD   — 单播到指定节点
 *   AA 02 FF FF LEN PAYLOAD            — 广播到所有节点
 *   AA 03                               — 查询 mesh 拓扑
 *
 * 上行 (网关 → 手机):
 *   AA 81 SRC_HI SRC_LO LEN PAYLOAD   — 某节点发来的数据
 *   AA 83 GW_HI GW_LO COUNT [ADDR_HI ADDR_LO HOPS]...  — 拓扑响应
 */
class MeshProtocol
{
public:
    static constexpr quint8 MAGIC = 0xAA;

    // 下行帧构造
    static QByteArray buildUnicast(quint16 dstAddr, const QByteArray &data);
    static QByteArray buildBroadcast(const QByteArray &data);
    static QByteArray buildTopoQuery();

    // 上行帧解析
    static UpstreamMessage parseNotification(const QByteArray &data);

    // 工具函数
    static QString toHexString(const QByteArray &data);
    static QString addrToHex4(quint16 addr);
    static QString decodePayloadToStringOrHex(const QByteArray &data);

private:
    static UpstreamMessage parseDataMessage(const QByteArray &data);
    static UpstreamMessage parseTopoResponse(const QByteArray &data);
};

Q_DECLARE_METATYPE(MeshNode)
Q_DECLARE_METATYPE(UpstreamMessage)

#endif // MESHPROTOCOL_H
