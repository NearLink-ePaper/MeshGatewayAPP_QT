#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QByteArray>
#include <QImage>
#include <QSize>

/**
 * 图片处理结果
 */
struct ProcessedImage {
    QImage   previewBitmap;   // 缩放后预览图 (用于 UI 显示)
    QByteArray imageData;     // 发送数据 (JPEG 压缩)
    int      dataSize;        // 发送数据大小
    int      packetCount;     // 分包数
    int      jpegQuality;     // JPEG 压缩质量
    quint8   imageMode;       // 取模模式 (2=JPEG)
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
     * 从裁剪后的图片处理: 缩放 → JPEG 极限有损压缩
     * WS63 端负责解压 + 抖动 + 刷屏
     */
    static ProcessedImage processFromCropped(const QImage &cropped, int targetW, int targetH,
                                              int jpegQuality = 40);
};

#endif // IMAGEUTILS_H
