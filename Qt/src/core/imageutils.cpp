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
 * 纯最近邻量化 → 4bpp nibble 数据（与 converterTo6color 完全一致）
 * 不做抖动，直接打包为固件接受的格式。
 */
static void quantizeToNibbles(const QImage &src, QByteArray &nibbles)
{
    int w = src.width();
    int h = src.height();
    nibbles.resize((w * h + 1) / 2);
    nibbles.fill(0);

    for (int y = 0; y < h; ++y) {
        const uchar *sl = src.constScanLine(y);
        for (int x = 0; x < w; ++x) {
            int ei = findNearestEpdIndex(sl[x*3], sl[x*3+1], sl[x*3+2]);
            int pos = y * w + x;
            if (pos % 2 == 0)
                nibbles[pos / 2] = (char)((ei & 0x0F) << 4);
            else
                nibbles[pos / 2] = (char)(nibbles[pos / 2] | (ei & 0x0F));
        }
    }
}

/**
 * NN 量化 → 轻微模糊 → RGB 预览图
 * 与设备发送数据的算法相同（最近邻），再叠加 1 像素 box blur 模拟
 * 墨水屏物理像素在正常观看距离下的视觉融合效果，避免 FS 抖动颗粒感。
 */
static QImage ditherEpdPreview(const QImage &src)
{
    int w = src.width();
    int h = src.height();

    // 第一步：最近邻量化（与发送数据完全一致）
    QImage quantized(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y) {
        const uchar *sl = src.constScanLine(y);
        uchar       *dl = quantized.scanLine(y);
        for (int x = 0; x < w; ++x) {
            int pi = findNearestPaletteIndex(sl[x*3], sl[x*3+1], sl[x*3+2]);
            dl[x*3+0] = (uchar)kEpdPalette[pi].r;
            dl[x*3+1] = (uchar)kEpdPalette[pi].g;
            dl[x*3+2] = (uchar)kEpdPalette[pi].b;
        }
    }

    // 第二步：1 像素 box blur — 模拟物理像素视觉融合，柔化色块边缘
    QImage blurred(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y) {
        const uchar *prev = quantized.constScanLine(y > 0 ? y-1 : 0);
        const uchar *curr = quantized.constScanLine(y);
        const uchar *next = quantized.constScanLine(y+1 < h ? y+1 : h-1);
        uchar *dl = blurred.scanLine(y);
        for (int x = 0; x < w; ++x) {
            int x0 = (x > 0 ? x-1 : 0) * 3, x1 = x*3, x2 = (x+1 < w ? x+1 : w-1) * 3;
            for (int c = 0; c < 3; ++c) {
                int sum = prev[x0+c] + prev[x1+c] + prev[x2+c]
                        + curr[x0+c] + curr[x1+c] + curr[x2+c]
                        + next[x0+c] + next[x1+c] + next[x2+c];
                dl[x*3+c] = (uchar)(sum / 9);
            }
        }
    }
    return blurred;
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
        quantizeToNibbles(scaled, nibbles);        // 纯NN → 设备数据
        QImage preview = ditherEpdPreview(scaled); // FS抖动 → 屏幕预览

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
    QImage quantized = ditherEpdPreview(jpegDecoded); // FS抖动预览
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
