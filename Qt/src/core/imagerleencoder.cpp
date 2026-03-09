#include "imagerleencoder.h"

QByteArray ImageRleEncoder::encode(const QByteArray &rawBytes, int totalPixels)
{
    // 展开为像素数组 (MSB first)
    QVector<int> pixels(totalPixels, 0);
    int idx = 0;
    for (int i = 0; i < rawBytes.size() && idx < totalPixels; ++i) {
        quint8 b = static_cast<quint8>(rawBytes.at(i));
        for (int bit = 7; bit >= 0 && idx < totalPixels; --bit) {
            pixels[idx++] = (b >> bit) & 1;
        }
    }

    QVector<quint8> output;
    int i = 0;

    while (i < pixels.size()) {
        int v = pixels[i];
        int run = 1;
        while (i + run < pixels.size() && pixels[i + run] == v)
            ++run;

        if (run >= 8) {
            // RLE: 连续 ≥ 8 个相同像素
            int count = run - 8;
            int low5 = count & 0x1F;
            bool hasMore = (count >> 5) > 0;
            output.append(static_cast<quint8>(0x80 | (v << 6) | ((hasMore ? 1 : 0) << 5) | low5));
            if (hasMore) {
                int remaining = count >> 5;
                while (remaining > 0) {
                    int part = remaining & 0x7F;
                    remaining >>= 7;
                    output.append(static_cast<quint8>(remaining > 0 ? (part | 0x80) : part));
                }
            }
            i += run;
        } else {
            // Literal: 打包 7 个像素
            int literal = 0;
            for (int k = 0; k < 7; ++k) {
                if (i + k < pixels.size()) {
                    literal |= (pixels[i + k] << (6 - k));
                }
            }
            output.append(static_cast<quint8>(literal)); // bit7 = 0
            i += 7;
        }
    }

    QByteArray result(output.size(), '\0');
    for (int j = 0; j < output.size(); ++j)
        result[j] = static_cast<char>(output[j]);
    return result;
}
