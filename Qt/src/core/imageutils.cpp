#include "imageutils.h"
#include "meshprotocol.h"
#include <QBuffer>
#include <QImageWriter>
#include <QColor>
#include <QTransform>
#include <QtMath>
#include <cmath>

/* === 7.3" 彩色墨水屏调色板 (6色) — 与 converterTo6color 完全一致 === */
struct EpdColor { int r, g, b; int epd_idx; };
static const EpdColor kEpdPalette[6] = {
    {   0,   0,   0, 0 },  // Black  → EPD index 0
    { 255, 255, 255, 1 },  // White  → EPD index 1
    { 255, 243,  56, 2 },  // Yellow → EPD index 2
    { 191,   0,   0, 3 },  // Red    → EPD index 3
    { 100,  64, 255, 5 },  // Blue   → EPD index 5 (index 4 reserved)
    {  67, 138,  28, 6 },  // Green  → EPD index 6
};

/* 最近邻量化：返回 EPD 硬件色彩索引 (0,1,2,3,5,6) */
static int findNearestEpdIndex(int r, int g, int b)
{
    int bestEpdIdx = 0;
    int bestDist = INT_MAX;
    for (int i = 0; i < 6; ++i) {
        int dr = r - kEpdPalette[i].r;
        int dg = g - kEpdPalette[i].g;
        int db = b - kEpdPalette[i].b;
        int dist = dr * dr + dg * dg + db * db;
        if (dist < bestDist) {
            bestDist = dist;
            bestEpdIdx = kEpdPalette[i].epd_idx;
        }
    }
    return bestEpdIdx;
}

/* 最近邻量化：返回调色板下标 (0-5)，用于获取 RGB 预览色 */
static int findNearestPaletteIndex(int r, int g, int b)
{
    int bestIdx = 0;
    int bestDist = INT_MAX;
    for (int i = 0; i < 6; ++i) {
        int dr = r - kEpdPalette[i].r;
        int dg = g - kEpdPalette[i].g;
        int db = b - kEpdPalette[i].b;
        int dist = dr * dr + dg * dg + db * db;
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

/**
 * 量化为墨水屏 6 色并生成预览图（无抖动，与 converterTo6color 算法一致）
 * 同时打包为 4bpp nibble 格式 (高nibble=左像素, 低nibble=右像素)
 * @param src     RGB888 QImage (portrait)
 * @param nibbles 输出：4bpp packed 数据
 * @return        预览用 RGB888 QImage
 */
static QImage quantizeToEpd(const QImage &src, QByteArray &nibbles)
{
    int w = src.width();
    int h = src.height();
    QImage out(w, h, QImage::Format_RGB888);
    nibbles.resize((w * h + 1) / 2);
    nibbles.fill(0);

    for (int y = 0; y < h; ++y) {
        const uchar *sl = src.constScanLine(y);
        uchar       *dl = out.scanLine(y);
        for (int x = 0; x < w; ++x) {
            int r = sl[x * 3 + 0];
            int g = sl[x * 3 + 1];
            int b = sl[x * 3 + 2];

            int pi = findNearestPaletteIndex(r, g, b);
            dl[x * 3 + 0] = (uchar)kEpdPalette[pi].r;
            dl[x * 3 + 1] = (uchar)kEpdPalette[pi].g;
            dl[x * 3 + 2] = (uchar)kEpdPalette[pi].b;

            int ei = findNearestEpdIndex(r, g, b);   // hardware index
            int bytePos = (y * w + x) / 2;
            if ((y * w + x) % 2 == 0)
                nibbles[bytePos] = (char)((ei & 0x0F) << 4);   // high nibble
            else
                nibbles[bytePos] = (char)(nibbles[bytePos] | (ei & 0x0F));  // low nibble
        }
    }
    return out;
}

QList<ImageResolution> ImageUtils::resolutions()
{
    return {
        {240, 360, QStringLiteral("240×360")},
        {480, 800, QStringLiteral("480×800")},
    };
}

ProcessedImage ImageUtils::processFromCropped(const QImage &cropped, int targetW, int targetH,
                                               int jpegQuality)
{
    // 缩放到目标分辨率 (portrait)
    QImage scaled = cropped.scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    scaled = scaled.convertToFormat(QImage::Format_RGB888);

    ProcessedImage result;
    result.jpegQuality = jpegQuality;

    // 4bpp nibble 模式：≤240×360 像素可放入设备缓冲区 (43 200 B < 148 KB)
    // 算法与 converterTo6color 完全一致：最近邻量化 + nibble 打包，无旋转
    // (固件 EPD_display_4bpp 内部做 90° CW 旋转)
    if (targetW * targetH <= 240 * 360) {
        QByteArray nibbles;
        QImage preview = quantizeToEpd(scaled, nibbles);

        result.previewBitmap = preview;
        result.imageData     = nibbles;
        result.dataSize      = nibbles.size();
        result.packetCount   = (nibbles.size() + MeshProtocol::IMG_PKT_PAYLOAD - 1)
                               / MeshProtocol::IMG_PKT_PAYLOAD;
        result.imageMode     = MeshProtocol::IMG_MODE_H_LSB;
        return result;
    }

    // JPEG 模式：大分辨率 (480×800) 原始 4bpp 超出设备内存，改用 JPEG 有损压缩
    // 旋转 90° CW → landscape，供设备流式 JPEG 解码
    QTransform rot;
    rot.rotate(90);
    QImage landscape = scaled.transformed(rot);

    QByteArray jpegData;
    QBuffer buffer(&jpegData);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, "jpeg");
    writer.setQuality(jpegQuality);
    writer.write(landscape);
    buffer.close();

    // 预览：JPEG 解码后量化到 EPD 调色板，再旋转回 portrait
    QImage jpegDecoded;
    jpegDecoded.loadFromData(jpegData, "JPEG");
    jpegDecoded = jpegDecoded.convertToFormat(QImage::Format_RGB888);
    QByteArray dummyNibbles;
    QImage quantized = quantizeToEpd(jpegDecoded, dummyNibbles);
    QTransform rotBack;
    rotBack.rotate(-90);

    result.previewBitmap = quantized.transformed(rotBack);
    result.imageData     = jpegData;
    result.dataSize      = jpegData.size();
    result.packetCount   = (jpegData.size() + MeshProtocol::IMG_PKT_PAYLOAD - 1)
                           / MeshProtocol::IMG_PKT_PAYLOAD;
    result.imageMode     = MeshProtocol::IMG_MODE_JPEG;
    return result;
}
