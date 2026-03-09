#include "imageutils.h"
#include "meshprotocol.h"
#include <QBuffer>
#include <QImageWriter>

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

    ProcessedImage result;
    result.previewBitmap = scaled;
    result.imageData     = jpegData;
    result.dataSize      = jpegData.size();
    result.packetCount   = pktCount;
    result.jpegQuality   = jpegQuality;
    result.imageMode     = MeshProtocol::IMG_MODE_JPEG;
    return result;
}
