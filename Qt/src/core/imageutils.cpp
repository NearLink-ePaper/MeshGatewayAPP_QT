#include "imageutils.h"
#include "imagerleencoder.h"
#include "meshprotocol.h"
#include <QColor>
#include <QtMath>

QList<ImageResolution> ImageUtils::resolutions()
{
    return {
        {240, 360, QStringLiteral("240×360")},
        {480, 800, QStringLiteral("480×800")},
    };
}

ProcessedImage ImageUtils::processFromCropped(const QImage &cropped, int targetW, int targetH)
{
    // 缩放到目标分辨率
    QImage scaled = cropped.scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    scaled = scaled.convertToFormat(QImage::Format_ARGB32);

    /* ── 1. 提取灰度矩阵（浮点，便于误差扩散）── */
    QVector<float> gray(targetW * targetH);
    for (int y = 0; y < targetH; ++y) {
        for (int x = 0; x < targetW; ++x) {
            QColor c(scaled.pixel(x, y));
            gray[y * targetW + x] = 0.299f * c.red() + 0.587f * c.green() + 0.114f * c.blue();
        }
    }

    /* ── 2. Floyd-Steinberg 抖动二值化 ── */
    QImage bwBitmap(targetW, targetH, QImage::Format_ARGB32);
    for (int y = 0; y < targetH; ++y) {
        for (int x = 0; x < targetW; ++x) {
            int idx = y * targetW + x;
            float oldPx = qBound(0.0f, gray[idx], 255.0f);
            float newPx = (oldPx >= 128.0f) ? 255.0f : 0.0f;
            float err = oldPx - newPx;
            bwBitmap.setPixel(x, y, newPx > 0 ? qRgb(255, 255, 255) : qRgb(0, 0, 0));

            // 扩散误差给相邻像素
            if (x + 1 < targetW)
                gray[idx + 1] += err * 7.0f / 16.0f;
            if (y + 1 < targetH) {
                if (x > 0)
                    gray[(y + 1) * targetW + (x - 1)] += err * 3.0f / 16.0f;
                gray[(y + 1) * targetW + x] += err * 5.0f / 16.0f;
                if (x + 1 < targetW)
                    gray[(y + 1) * targetW + (x + 1)] += err * 1.0f / 16.0f;
            }
        }
    }

    /* ── 3. 1bpp 取模 (MSB-first: pixel 0 → bit 7) ── */
    int bytesPerRow = (targetW + 7) / 8;
    int totalBytes = bytesPerRow * targetH;
    QByteArray data(totalBytes, '\0');
    for (int y = 0; y < targetH; ++y) {
        for (int x = 0; x < targetW; ++x) {
            QRgb pixel = bwBitmap.pixel(x, y);
            bool isBlack = (pixel & 0x00FFFFFF) == 0;
            if (isBlack) {
                int byteIndex = y * bytesPerRow + (x / 8);
                int bitIndex = 7 - (x % 8);  // MSB-first
                data[byteIndex] = static_cast<char>(
                    static_cast<quint8>(data.at(byteIndex)) | (1 << bitIndex));
            }
        }
    }

    // RLE 压缩: 仅当压缩后更小时使用
    int totalPixels = targetW * targetH;
    QByteArray compressed = ImageRleEncoder::encode(data, totalPixels);
    bool useRle = compressed.size() < totalBytes;
    QByteArray sendData = useRle ? compressed : data;
    quint8 imgMode = useRle ? MeshProtocol::IMG_MODE_RLE : MeshProtocol::IMG_MODE_H_LSB;

    int pktCount = (sendData.size() + MeshProtocol::IMG_PKT_PAYLOAD - 1) / MeshProtocol::IMG_PKT_PAYLOAD;

    ProcessedImage result;
    result.previewBitmap = bwBitmap;
    result.imageData     = sendData;
    result.dataSize      = sendData.size();
    result.packetCount   = pktCount;
    result.rawSize       = totalBytes;
    result.isCompressed  = useRle;
    result.imageMode     = imgMode;
    return result;
}
