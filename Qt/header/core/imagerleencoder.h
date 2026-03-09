#ifndef IMAGERLEENCODER_H
#define IMAGERLEENCODER_H

#include <QByteArray>
#include <QVector>

/**
 * RLE (游程编码) 编码器 — 用于 1bpp 二值图像压缩
 *
 * 编码格式与固件端 image_rle.c 解码器完全匹配:
 *   Literal (0xxxxxxx): 7 个原始像素
 *   RLE     (1vNccccc): 连续 ≥8 个相同像素, varint 编码长度
 *
 * 像素按 MSB-first 从字节中提取 (bit7 = 第一个像素),
 * 与 C 端解码器写入顺序一致, 保证编解码 round-trip 字节一致。
 */
class ImageRleEncoder
{
public:
    /**
     * 对 1bpp 像素数据进行 RLE 压缩
     *
     * @param rawBytes    原始 1bpp 数据 (每字节 8 像素, MSB first)
     * @param totalPixels 总像素数 (width × height)
     * @return 压缩后的字节数组
     */
    static QByteArray encode(const QByteArray &rawBytes, int totalPixels);
};

#endif // IMAGERLEENCODER_H
