/**
 * @file  jpeg_decoder.c
 * @brief JPEG 解码 + Floyd-Steinberg 6色抖动 → 4bpp 墨水屏输出
 *
 * @details
 *   逐 MCU 行流式处理，内存占用极小:
 *     - TJpgDec 逐 MCU 输出 RGB 像素到"行条带缓冲区"(width × mcu_h × 3)
 *     - 每攒满一个 MCU 行 (检测到 rect->top 变化) 立即执行 Floyd-Steinberg 抖动
 *     - 抖动结果直接写入 4bpp 输出缓冲区
 *     - 误差缓冲区仅保留两行 (当前行 + 下一行)
 *
 *   峰值动态内存 (~33KB for 480px wide):
 *     - TJpgDec 工作池: 4KB
 *     - MCU 行条带 RGB: width × 16 × 3 ≈ 23KB
 *     - 误差缓冲: 2 × width × 3 × 2 ≈ 6KB
 */

#include "jpeg_decoder.h"
#include "tjpgd.h"
#include "osal_debug.h"
#include "soc_osal.h"
#include "securec.h"
#include <string.h>

#define JPEG_LOG "[jpeg dec]"

/* TJpgDec 工作内存池大小 (字节) */
#define TJPGD_POOL_SIZE  4096

/* 7.3" 彩色墨水屏 6 色调色板 (RGB) */
typedef struct { uint8_t r, g, b; } epd_rgb_t;
static const epd_rgb_t kPalette[6] = {
    {   0,   0,   0 },  /* 0: Black  */
    { 255, 255, 255 },  /* 1: White  */
    { 255, 255,   0 },  /* 2: Yellow */
    { 255,   0,   0 },  /* 3: Red    */
    {   0,   0, 255 },  /* 4: Blue   */
    {   0, 255,   0 },  /* 5: Green  */
};
/* 调色板索引 → EPD 4bpp 颜色码映射 */
static const uint8_t kPaletteToEpd[6] = { 0x0, 0x1, 0x2, 0x3, 0x5, 0x6 };

/* ── 内存流输入回调 ────────────────────────────────────── */
typedef struct {
    const uint8_t *data;
    uint32_t       size;
    uint32_t       pos;
} mem_stream_t;

static unsigned int jpeg_input_func(JDEC *jd, uint8_t *buff, unsigned int nbyte)
{
    mem_stream_t *s = (mem_stream_t *)jd->device;
    unsigned int remain = s->size - s->pos;
    if (nbyte > remain) nbyte = remain;
    if (buff) {
        (void)memcpy_s(buff, nbyte, s->data + s->pos, nbyte);
    }
    s->pos += nbyte;
    return nbyte;
}

/* ── On-the-fly 抖动上下文 (全局单例) ──────────────────── */
static struct {
    uint8_t  *out_4bpp;       /* 4bpp 输出缓冲区 (外部提供) */
    uint16_t  img_w;          /* 图像宽度 */
    uint16_t  img_h;          /* 图像高度 */
    uint8_t  *strip_rgb;      /* MCU 行条带 RGB 缓冲 (img_w × mcu_h × 3) */
    uint16_t  strip_top;      /* 当前条带顶部 Y (0xFFFF = 未开始) */
    uint16_t  mcu_h;          /* MCU 块高度 (8 或 16) */
    int16_t  *err_cur;        /* 误差扩散: 当前行 (img_w × 3) */
    int16_t  *err_nxt;        /* 误差扩散: 下一行 (img_w × 3) */
} s_dctx;

static inline int clamp8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

static uint8_t find_nearest_epd(int r, int g, int b)
{
    uint32_t best_dist = 0xFFFFFFFF;
    uint8_t  best_idx = 0;
    uint8_t  i;
    for (i = 0; i < 6; i++) {
        int dr = r - (int)kPalette[i].r;
        int dg = g - (int)kPalette[i].g;
        int db = b - (int)kPalette[i].b;
        uint32_t dist = (uint32_t)(dr*dr + dg*dg + db*db);
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = i;
        }
    }
    return best_idx;
}

/**
 * 对指定行做 Floyd-Steinberg 抖动并写入 4bpp 输出
 * @param strip_local_y  该行在条带缓冲中的局部行号
 * @param abs_y          该行在整幅图像中的绝对 Y 坐标
 */
static void dither_one_row(uint16_t strip_local_y, uint16_t abs_y)
{
    uint16_t w = s_dctx.img_w;
    int16_t *ec = s_dctx.err_cur;
    int16_t *en = s_dctx.err_nxt;
    uint8_t *rgb_row = &s_dctx.strip_rgb[(uint32_t)strip_local_y * w * 3];
    uint8_t *out_row = &s_dctx.out_4bpp[(uint32_t)abs_y * (w / 2)];
    uint16_t x;
    int16_t *tmp;

    /* 清零下一行误差 */
    (void)memset_s(en, (uint32_t)w * 3 * sizeof(int16_t), 0,
                   (uint32_t)w * 3 * sizeof(int16_t));

    for (x = 0; x < w; x++) {
        int oldR = clamp8((int)rgb_row[x*3+0] + ec[x*3+0]);
        int oldG = clamp8((int)rgb_row[x*3+1] + ec[x*3+1]);
        int oldB = clamp8((int)rgb_row[x*3+2] + ec[x*3+2]);

        uint8_t ci = find_nearest_epd(oldR, oldG, oldB);
        uint8_t epd_code = kPaletteToEpd[ci];

        /* 写入 4bpp packed (高 nibble = 偶像素, 低 nibble = 奇像素) */
        if (x & 1) {
            out_row[x / 2] |= epd_code;
        } else {
            out_row[x / 2] = (uint8_t)(epd_code << 4);
        }

        /* 误差 */
        int errR = oldR - (int)kPalette[ci].r;
        int errG = oldG - (int)kPalette[ci].g;
        int errB = oldB - (int)kPalette[ci].b;

        if (x + 1 < w) {
            ec[(x+1)*3+0] += (int16_t)(errR * 7 / 16);
            ec[(x+1)*3+1] += (int16_t)(errG * 7 / 16);
            ec[(x+1)*3+2] += (int16_t)(errB * 7 / 16);
        }
        if (abs_y + 1 < s_dctx.img_h && x > 0) {
            en[(x-1)*3+0] += (int16_t)(errR * 3 / 16);
            en[(x-1)*3+1] += (int16_t)(errG * 3 / 16);
            en[(x-1)*3+2] += (int16_t)(errB * 3 / 16);
        }
        if (abs_y + 1 < s_dctx.img_h) {
            en[x*3+0] += (int16_t)(errR * 5 / 16);
            en[x*3+1] += (int16_t)(errG * 5 / 16);
            en[x*3+2] += (int16_t)(errB * 5 / 16);
        }
        if (abs_y + 1 < s_dctx.img_h && x + 1 < w) {
            en[(x+1)*3+0] += (int16_t)(errR * 1 / 16);
            en[(x+1)*3+1] += (int16_t)(errG * 1 / 16);
            en[(x+1)*3+2] += (int16_t)(errB * 1 / 16);
        }
    }

    /* 交换 cur/nxt */
    tmp = s_dctx.err_cur;
    s_dctx.err_cur = s_dctx.err_nxt;
    s_dctx.err_nxt = tmp;
}

/** 抖动当前条带中的所有行 */
static void dither_strip(void)
{
    uint16_t strip_h = s_dctx.mcu_h;
    uint16_t row;
    if (s_dctx.strip_top + strip_h > s_dctx.img_h) {
        strip_h = s_dctx.img_h - s_dctx.strip_top;
    }
    for (row = 0; row < strip_h; row++) {
        dither_one_row(row, s_dctx.strip_top + row);
    }
}

/* ── TJpgDec MCU 输出回调 (on-the-fly) ───────────────── */
static int jpeg_output_cb(JDEC *jd, void *bitmap, JRECT *rect)
{
    uint8_t *src = (uint8_t *)bitmap;
    uint16_t bw = rect->right - rect->left + 1;
    uint16_t copy_w, local_y, y;
    uint32_t dst_off, src_off;
    uint16_t strip_h;
    (void)jd;

    /* 检测是否进入了新的 MCU 行 → 先抖动上一个条带 */
    if (s_dctx.strip_top != 0xFFFF &&
        rect->top >= s_dctx.strip_top + s_dctx.mcu_h) {
        dither_strip();
        s_dctx.strip_top = 0xFFFF;
    }

    /* 初始化新条带 */
    if (s_dctx.strip_top == 0xFFFF) {
        s_dctx.strip_top = rect->top;
        strip_h = s_dctx.mcu_h;
        if (s_dctx.strip_top + strip_h > s_dctx.img_h) {
            strip_h = s_dctx.img_h - s_dctx.strip_top;
        }
        (void)memset_s(s_dctx.strip_rgb,
                       (uint32_t)s_dctx.img_w * strip_h * 3,
                       0xFF,
                       (uint32_t)s_dctx.img_w * strip_h * 3);
    }

    /* 将 MCU 块的 RGB 像素拷贝到条带缓冲区对应位置 */
    for (y = rect->top; y <= rect->bottom && y < s_dctx.img_h; y++) {
        local_y = y - s_dctx.strip_top;
        dst_off = (uint32_t)local_y * s_dctx.img_w * 3 + (uint32_t)rect->left * 3;
        src_off = (uint32_t)(y - rect->top) * bw * 3;
        copy_w = bw;
        if (rect->left + copy_w > s_dctx.img_w) {
            copy_w = s_dctx.img_w - rect->left;
        }
        (void)memcpy_s(&s_dctx.strip_rgb[dst_off], copy_w * 3,
                       &src[src_off], copy_w * 3);
    }

    return 1;  /* continue */
}

/* ── 公共 API ─────────────────────────────────────────── */
bool jpeg_decode_to_epd(const uint8_t *jpeg_data, uint32_t jpeg_size,
                        uint8_t *out_4bpp, uint32_t out_cap,
                        uint16_t *out_width, uint16_t *out_height)
{
    JDEC jdec;
    JRESULT rc;
    uint8_t *pool = NULL;
    bool result = false;
    uint16_t w = 0, h = 0;
    mem_stream_t stream;
    uint32_t err_row_bytes = 0;
    uint32_t strip_bytes = 0;

    /* 分配 TJpgDec 工作池 */
    pool = (uint8_t *)osal_vmalloc(TJPGD_POOL_SIZE);
    if (!pool) {
        osal_printk("%s pool alloc failed (%d)\r\n", JPEG_LOG, TJPGD_POOL_SIZE);
        return false;
    }

    /* 准备内存流 */
    stream.data = jpeg_data;
    stream.size = jpeg_size;
    stream.pos  = 0;

    /* 解析 JPEG 头 */
    rc = jd_prepare(&jdec, jpeg_input_func, pool, TJPGD_POOL_SIZE, &stream);
    if (rc != JDR_OK) {
        osal_printk("%s jd_prepare failed: %d\r\n", JPEG_LOG, rc);
        goto cleanup;
    }

    w = (uint16_t)jdec.width;
    h = (uint16_t)jdec.height;
    osal_printk("%s JPEG: %dx%d, %d bytes\r\n", JPEG_LOG, w, h, jpeg_size);

    if ((uint32_t)(w / 2) * h > out_cap) {
        osal_printk("%s output buf too small: need %lu, have %lu\r\n",
                    JPEG_LOG, (unsigned long)((uint32_t)(w/2)*h), (unsigned long)out_cap);
        goto cleanup;
    }

    /* 初始化 on-the-fly 抖动上下文 */
    (void)memset_s(&s_dctx, sizeof(s_dctx), 0, sizeof(s_dctx));
    s_dctx.out_4bpp  = out_4bpp;
    s_dctx.img_w     = w;
    s_dctx.img_h     = h;
    s_dctx.mcu_h     = (uint16_t)(jdec.msy * 8);  /* MCU 行高: 8 or 16 */
    s_dctx.strip_top = 0xFFFF;                     /* 未开始标记 */

    /* 分配 MCU 行条带 RGB 缓冲 */
    strip_bytes = (uint32_t)w * s_dctx.mcu_h * 3;
    s_dctx.strip_rgb = (uint8_t *)osal_vmalloc(strip_bytes);
    if (!s_dctx.strip_rgb) {
        osal_printk("%s strip alloc failed (%lu)\r\n", JPEG_LOG, (unsigned long)strip_bytes);
        goto cleanup;
    }

    /* 分配误差扩散缓冲 (2行) */
    err_row_bytes = (uint32_t)w * 3 * sizeof(int16_t);
    s_dctx.err_cur = (int16_t *)osal_vmalloc(err_row_bytes);
    s_dctx.err_nxt = (int16_t *)osal_vmalloc(err_row_bytes);
    if (!s_dctx.err_cur || !s_dctx.err_nxt) {
        osal_printk("%s err alloc failed\r\n", JPEG_LOG);
        goto cleanup;
    }
    (void)memset_s(s_dctx.err_cur, err_row_bytes, 0, err_row_bytes);
    (void)memset_s(s_dctx.err_nxt, err_row_bytes, 0, err_row_bytes);

    /* 清零 4bpp 输出 (0x11 = White|White) */
    (void)memset_s(out_4bpp, out_cap, 0x11, (uint32_t)(w / 2) * h);

    /* 解码 — 回调中自动逐 MCU 行抖动 */
    rc = jd_decomp(&jdec, jpeg_output_cb, 0);
    if (rc != JDR_OK) {
        osal_printk("%s jd_decomp failed: %d\r\n", JPEG_LOG, rc);
        goto cleanup;
    }

    /* 处理最后一个 MCU 行条带 */
    if (s_dctx.strip_top != 0xFFFF) {
        dither_strip();
    }

    *out_width  = w;
    *out_height = h;
    result = true;
    osal_printk("%s done: %dx%d → 4bpp %lu bytes\r\n",
                JPEG_LOG, w, h, (unsigned long)((uint32_t)(w / 2) * h));

cleanup:
    if (s_dctx.strip_rgb) { osal_vfree(s_dctx.strip_rgb); }
    if (s_dctx.err_cur)   { osal_vfree(s_dctx.err_cur); }
    if (s_dctx.err_nxt)   { osal_vfree(s_dctx.err_nxt); }
    (void)memset_s(&s_dctx, sizeof(s_dctx), 0, sizeof(s_dctx));
    if (pool) { osal_vfree(pool); }
    return result;
}
