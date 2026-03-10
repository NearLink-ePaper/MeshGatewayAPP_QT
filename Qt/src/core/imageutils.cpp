#include "imageutils.h"
#include "meshprotocol.h"
#include <QVector>

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

static inline int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

/**
 * Floyd-Steinberg 抖动 → 同时输出 4bpp nibble 数据和 RGB 预览图
 * 设备数据与客户端预览完全一致，避免色彩块状感（颗粒感）。
 * @param src     RGB888 QImage (portrait)
 * @param nibbles 输出：发送给设备的 4bpp packed nibble 数据
 * @return        屏幕预览用 RGB888 QImage（与设备显示完全一致）
 */
static QImage quantizeToNibbles(const QImage &src, QByteArray &nibbles)
{
    int w = src.width();
    int h = src.height();
    nibbles.resize((w * h + 1) / 2);
    nibbles.fill(0);

    QVector<int> errR(w * h), errG(w * h), errB(w * h);
    for (int y = 0; y < h; ++y) {
        const uchar *sl = src.constScanLine(y);
        for (int x = 0; x < w; ++x) {
            int i = y * w + x;
            errR[i] = sl[x*3+0];
            errG[i] = sl[x*3+1];
            errB[i] = sl[x*3+2];
        }
    }

    QImage preview(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y) {
        uchar *dl = preview.scanLine(y);
        for (int x = 0; x < w; ++x) {
            int i = y * w + x;
            int oldR = clamp255(errR[i]);
            int oldG = clamp255(errG[i]);
            int oldB = clamp255(errB[i]);

            int pi = findNearestPaletteIndex(oldR, oldG, oldB);
            int newR = kEpdPalette[pi].r, newG = kEpdPalette[pi].g, newB = kEpdPalette[pi].b;
            dl[x*3+0] = (uchar)newR; dl[x*3+1] = (uchar)newG; dl[x*3+2] = (uchar)newB;

            // 打包 EPD 硬件索引
            int ei = kEpdPalette[pi].epd_idx;
            if (i % 2 == 0)
                nibbles[i / 2] = (char)((ei & 0x0F) << 4);
            else
                nibbles[i / 2] = (char)(nibbles[i / 2] | (ei & 0x0F));

            // Floyd-Steinberg 误差扩散
            int eR = oldR - newR, eG = oldG - newG, eB = oldB - newB;
            if (x+1 < w) { errR[i+1]+=eR*7/16; errG[i+1]+=eG*7/16; errB[i+1]+=eB*7/16; }
            if (y+1 < h) {
                if (x>0) { errR[i+w-1]+=eR*3/16; errG[i+w-1]+=eG*3/16; errB[i+w-1]+=eB*3/16; }
                errR[i+w]+=eR*5/16; errG[i+w]+=eG*5/16; errB[i+w]+=eB*5/16;
                if (x+1<w) { errR[i+w+1]+=eR/16; errG[i+w+1]+=eG/16; errB[i+w+1]+=eB/16; }
            }
        }
    }
    return preview;
}

/* ditherEpdPreview: 预览与数据共用同一 FS 路径，保持一致 */
static QImage ditherEpdPreview(const QImage &src)
{
    QByteArray dummy;
    return quantizeToNibbles(src, dummy);
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

    // 4bpp RAW 模式：FS 抖动量化 → nibble 打包，无旋转
    // 固件 EPD_display_4bpp 内部做 90° CW 旋转
    // SRAM 606KB，480×800 仅需 192KB，完全支持
    QByteArray nibbles;
    QImage preview = quantizeToNibbles(scaled, nibbles);

    result.previewBitmap = preview;
    result.imageData     = nibbles;
    result.dataSize      = nibbles.size();
    result.packetCount   = (nibbles.size() + MeshProtocol::IMG_PKT_PAYLOAD - 1)
                           / MeshProtocol::IMG_PKT_PAYLOAD;
    result.imageMode     = MeshProtocol::IMG_MODE_H_LSB;
    return result;
}
