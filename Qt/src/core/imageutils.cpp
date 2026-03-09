#include "imageutils.h"
#include "meshprotocol.h"
#include <QBuffer>
#include <QImageWriter>
#include <QColor>
#include <QtMath>
#include <cmath>

/* === 7.3” 彩色墨水屏调色板 (6色) === */
struct EpdColor { int r, g, b; };
static const EpdColor kEpdPalette[6] = {
    {   0,   0,   0 },  // Black
    { 255, 255, 255 },  // White
    { 255, 255,   0 },  // Yellow
    { 255,   0,   0 },  // Red
    {   0,   0, 255 },  // Blue
    {   0, 255,   0 },  // Green
};

static int findNearestColor(int r, int g, int b)
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
 * 模拟墨水屏 6 色 Floyd-Steinberg 抖动
 * 输入: RGB888 QImage
 * 输出: 抖动后的 RGB QImage (仅含调色板中的6种颜色)
 */
static QImage ditherToEpdPalette(const QImage &src)
{
    int w = src.width();
    int h = src.height();
    // 工作缓冲区：存 int 避免截断误差
    QVector<int> bufR(w * h), bufG(w * h), bufB(w * h);
    for (int y = 0; y < h; ++y) {
        const uchar *line = src.constScanLine(y);
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            bufR[idx] = line[x * 3 + 0];
            bufG[idx] = line[x * 3 + 1];
            bufB[idx] = line[x * 3 + 2];
        }
    }

    QImage out(w, h, QImage::Format_RGB888);
    for (int y = 0; y < h; ++y) {
        uchar *outLine = out.scanLine(y);
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            int oldR = clamp255(bufR[idx]);
            int oldG = clamp255(bufG[idx]);
            int oldB = clamp255(bufB[idx]);

            int ci = findNearestColor(oldR, oldG, oldB);
            int newR = kEpdPalette[ci].r;
            int newG = kEpdPalette[ci].g;
            int newB = kEpdPalette[ci].b;

            outLine[x * 3 + 0] = (uchar)newR;
            outLine[x * 3 + 1] = (uchar)newG;
            outLine[x * 3 + 2] = (uchar)newB;

            int errR = oldR - newR;
            int errG = oldG - newG;
            int errB = oldB - newB;

            // Floyd-Steinberg 误差扩散
            if (x + 1 < w) {
                bufR[idx + 1] += errR * 7 / 16;
                bufG[idx + 1] += errG * 7 / 16;
                bufB[idx + 1] += errB * 7 / 16;
            }
            if (y + 1 < h) {
                if (x > 0) {
                    bufR[(y+1)*w + x-1] += errR * 3 / 16;
                    bufG[(y+1)*w + x-1] += errG * 3 / 16;
                    bufB[(y+1)*w + x-1] += errB * 3 / 16;
                }
                bufR[(y+1)*w + x] += errR * 5 / 16;
                bufG[(y+1)*w + x] += errG * 5 / 16;
                bufB[(y+1)*w + x] += errB * 5 / 16;
                if (x + 1 < w) {
                    bufR[(y+1)*w + x+1] += errR * 1 / 16;
                    bufG[(y+1)*w + x+1] += errG * 1 / 16;
                    bufB[(y+1)*w + x+1] += errB * 1 / 16;
                }
            }
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
    // 缩放到目标分辨率
    QImage scaled = cropped.scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    scaled = scaled.convertToFormat(QImage::Format_RGB888);

    // JPEG 极限有损压缩
    QByteArray jpegData;
    QBuffer buffer(&jpegData);
    buffer.open(QIODevice::WriteOnly);
    QImageWriter writer(&buffer, "jpeg");
    writer.setQuality(jpegQuality);
    writer.write(scaled);
    buffer.close();

    int pktCount = (jpegData.size() + MeshProtocol::IMG_PKT_PAYLOAD - 1) / MeshProtocol::IMG_PKT_PAYLOAD;

    // 解码 JPEG 并模拟墨水屏 6 色抖动作为预览
    QImage jpegDecoded;
    jpegDecoded.loadFromData(jpegData, "JPEG");
    jpegDecoded = jpegDecoded.convertToFormat(QImage::Format_RGB888);
    QImage dithered = ditherToEpdPalette(jpegDecoded);

    ProcessedImage result;
    result.previewBitmap = dithered;
    result.imageData     = jpegData;
    result.dataSize      = jpegData.size();
    result.packetCount   = pktCount;
    result.jpegQuality   = jpegQuality;
    result.imageMode     = MeshProtocol::IMG_MODE_JPEG;
    return result;
}
