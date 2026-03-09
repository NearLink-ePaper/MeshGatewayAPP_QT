#ifndef MESHPROTOCOL_H
#define MESHPROTOCOL_H

#include <QByteArray>
#include <QString>
#include <QList>
#include <QMap>
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
    enum Type {
        Unknown,
        DataFromNode,
        Topology,
        ImageAck,
        ImageResult,
        ImageMissing,
        ImageProgress,
        MulticastProgress
    };
    Type type = Unknown;

    // DataFromNode 字段
    quint16    srcAddr  = 0;
    QByteArray payload;

    // Topology 字段
    quint16          gatewayAddr = 0;
    QList<MeshNode>  nodes;

    // ImageAck 字段 (0x85)
    quint8  ackStatus = 0;
    quint16 ackSeq    = 0;

    // ImageResult 字段 (0x86)
    quint8  imgResultStatus = 0;

    // ImageMissing 字段 (0x87)
    quint16 totalMissing = 0;

    // ImageProgress 字段 (0x89)
    quint8  progressPhase = 0;
    quint16 progressRxCount = 0;
    quint16 progressTotal   = 0;

    // MulticastProgress 字段 (0x8A)
    quint8  mcastCompleted   = 0;
    quint8  mcastTotalTargets = 0;
    quint16 mcastLatestAddr   = 0;
    quint8  mcastLatestStatus = 0;
};

/**
 * BLE-Mesh 网关通信协议编解码
 *
 * 下行 (手机 → 网关):
 *   AA 01 DST_HI DST_LO LEN PAYLOAD   — 单播到指定节点
 *   AA 02 FF FF LEN PAYLOAD            — 广播到所有节点
 *   AA 03                               — 查询 mesh 拓扑
 *   AA 04 DST(2) TOTAL(2) PKT(2) W(2) H(2) MODE XFER  — 图片 START
 *   AA 05 DST(2) SEQ(2) LEN PAYLOAD   — 图片数据分包
 *   AA 06 DST(2) CRC(2)               — 图片 END
 *   AA 07 DST(2)                       — 取消图片传输
 *   AA 0A N ADDR1(2)...ADDRn(2) TOTAL(2) PKT(2) W(2) H(2) MODE — 组播 START
 *
 * 上行 (网关 → 手机):
 *   AA 81 SRC(2) LEN PAYLOAD           — 某节点发来的数据
 *   AA 83 GW(2) COUNT [ADDR(2) HOPS]...— 拓扑响应
 *   AA 85 SRC(2) STATUS SEQ(2)          — 图片分包 ACK
 *   AA 86 SRC(2) STATUS                 — 图片传输结果
 *   AA 87 SRC(2) MISS_HI MISS_LO BITMAP — 缺包位图
 *   AA 89 SRC(2) PHASE RX(2) TOTAL(2)   — 网关流控进度
 *   AA 8A DONE TOTAL ADDR(2) STATUS     — 组播进度通知
 */
class MeshProtocol
{
public:
    static constexpr quint8 MAGIC = 0xAA;

    /* ── 下行命令码 ── */
    static constexpr quint8 CMD_UNICAST        = 0x01;
    static constexpr quint8 CMD_BROADCAST      = 0x02;
    static constexpr quint8 CMD_TOPO_QUERY     = 0x03;
    static constexpr quint8 CMD_IMG_START      = 0x04;
    static constexpr quint8 CMD_IMG_DATA       = 0x05;
    static constexpr quint8 CMD_IMG_END        = 0x06;
    static constexpr quint8 CMD_IMG_CANCEL     = 0x07;
    static constexpr quint8 CMD_IMG_MCAST_START = 0x0A;

    /* ── 上行命令码 ── */
    static constexpr quint8 CMD_DATA_UP        = 0x81;
    static constexpr quint8 CMD_TOPO_RESP      = 0x83;
    static constexpr quint8 CMD_IMG_ACK        = 0x85;
    static constexpr quint8 CMD_IMG_RESULT     = 0x86;
    static constexpr quint8 CMD_IMG_MISSING    = 0x87;
    static constexpr quint8 CMD_IMG_PROGRESS   = 0x89;
    static constexpr quint8 CMD_IMG_MCAST_PROGRESS = 0x8A;

    /* ── 图片 ACK 状态 ── */
    static constexpr quint8 IMG_ACK_OK     = 0x00;
    static constexpr quint8 IMG_ACK_RESEND = 0x01;
    static constexpr quint8 IMG_ACK_DONE   = 0xFF;

    /* ── 图片传输结果 ── */
    static constexpr quint8 IMG_RESULT_OK      = 0x00;
    static constexpr quint8 IMG_RESULT_OOM     = 0x01;
    static constexpr quint8 IMG_RESULT_TIMEOUT = 0x02;
    static constexpr quint8 IMG_RESULT_CANCEL  = 0x03;
    static constexpr quint8 IMG_RESULT_CRC_ERR = 0x04;

    /* ── 取模模式 ── */
    static constexpr quint8 IMG_MODE_H_LSB = 0x00;
    static constexpr quint8 IMG_MODE_RLE   = 0x01;
    static constexpr quint8 IMG_MODE_JPEG  = 0x02;

    /* ── 传输模式 ── */
    static constexpr quint8 IMG_XFER_FAST = 0x00;
    static constexpr quint8 IMG_XFER_ACK  = 0x01;

    /* ── 分包参数 ── */
    static constexpr int IMG_PKT_PAYLOAD = 200;

    // 下行帧构造
    static QByteArray buildUnicast(quint16 dstAddr, const QByteArray &data);
    static QByteArray buildBroadcast(const QByteArray &data);
    static QByteArray buildTopoQuery();
    static QByteArray buildImageStart(quint16 dstAddr, quint16 totalBytes, quint16 pktCount,
                                      quint16 width, quint16 height,
                                      quint8 mode = IMG_MODE_H_LSB,
                                      quint8 xfer = IMG_XFER_FAST);
    static QByteArray buildImageData(quint16 dstAddr, quint16 seq, const QByteArray &data);
    static QByteArray buildImageEnd(quint16 dstAddr, quint16 crc16);
    static QByteArray buildImageCancel(quint16 dstAddr);
    static QByteArray buildImageMulticastStart(const QList<quint16> &targets,
                                               quint16 totalBytes, quint16 pktCount,
                                               quint16 width, quint16 height,
                                               quint8 mode = IMG_MODE_H_LSB);

    // 上行帧解析
    static UpstreamMessage parseNotification(const QByteArray &data);

    // CRC-16/CCITT
    static quint16 crc16(const QByteArray &data);

    // 工具函数
    static QString toHexString(const QByteArray &data);
    static QString addrToHex4(quint16 addr);
    static QString decodePayloadToStringOrHex(const QByteArray &data);
    static QString imageResultToString(quint8 status);

private:
    static UpstreamMessage parseDataMessage(const QByteArray &data);
    static UpstreamMessage parseTopoResponse(const QByteArray &data);
    static UpstreamMessage parseImageAck(const QByteArray &data);
    static UpstreamMessage parseImageResult(const QByteArray &data);
    static UpstreamMessage parseImageMissing(const QByteArray &data);
    static UpstreamMessage parseImageProgress(const QByteArray &data);
    static UpstreamMessage parseMulticastProgress(const QByteArray &data);
};

Q_DECLARE_METATYPE(MeshNode)
Q_DECLARE_METATYPE(UpstreamMessage)

#endif // MESHPROTOCOL_H
