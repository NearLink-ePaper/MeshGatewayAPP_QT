#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QByteArray>
#include <QImage>
#include <QSize>

/**
 * 图片处理结果
 */
struct ProcessedImage {
    QImage   previewBitmap;   // 二值化预览图 (用于 UI 显示)
    QByteArray imageData;     // 发送数据 (可能 RLE 压缩)
    int      dataSize;        // 发送数据大小
    int      packetCount;     // 分包数
    int      rawSize;         // 原始未压缩大小
    bool     isCompressed;    // 是否使用了 RLE 压缩
    quint8   imageMode;       // 取模模式 (0=H_LSB, 1=RLE)
};

/**
 * 图片分辨率选项
 */
struct ImageResolution {
    int width;
    int height;
    QString label;
};

/**
 * 图片处理工具
 */
class ImageUtils
{
public:
    // 预定义分辨率选项
    static QList<ImageResolution> resolutions();

    /**
     * 从裁剪后的图片处理: 缩放 → Floyd-Steinberg 抖动二值化 → 取模 (MSB-first)
     * 自动选择是否 RLE 压缩
     */
    static ProcessedImage processFromCropped(const QImage &cropped, int targetW, int targetH);
};

#endif // IMAGEUTILS_H
