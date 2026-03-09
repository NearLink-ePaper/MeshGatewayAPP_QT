/**
 * @file  jpeg_decoder.h
 * @brief JPEG 解码 + Floyd-Steinberg 6色抖动 → 4bpp 墨水屏输出
 *
 * @details
 *   利用 TJpgDec 逐 MCU 解码 JPEG 数据，在输出回调中实时执行
 *   Floyd-Steinberg 误差扩散抖动，将 RGB 像素量化为 7.3" 彩色
 *   墨水屏的 6 色调色板 (Black/White/Yellow/Red/Blue/Green)，
 *   最终输出 4bpp packed 格式 (每字节 2 像素) 可直接送 SPI 刷屏。
 *
 *   内存占用：
 *     - TJpgDec 工作池: ~4KB
 *     - 误差缓冲区: 2 行 × width × 3通道 × sizeof(int16_t) ≈ 9.6KB (800px)
 *     - 4bpp 输出缓冲区: width/2 × height = 192KB (外部提供)
 */
#ifndef JPEG_DECODER_H
#define JPEG_DECODER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  将 JPEG 数据解码并抖动为 4bpp 墨水屏格式
 *
 * @param  jpeg_data   JPEG 压缩数据指针
 * @param  jpeg_size   JPEG 数据大小（字节）
 * @param  out_4bpp    输出缓冲区，大小 ≥ (width/2)*height 字节
 * @param  out_cap     输出缓冲区容量
 * @param  out_width   [out] 解码后图像宽度
 * @param  out_height  [out] 解码后图像高度
 * @return true=成功, false=解码失败或缓冲区不足
 */
bool jpeg_decode_to_epd(const uint8_t *jpeg_data, uint32_t jpeg_size,
                        uint8_t *out_4bpp, uint32_t out_cap,
                        uint16_t *out_width, uint16_t *out_height);

#ifdef __cplusplus
}
#endif

#endif /* JPEG_DECODER_H */
