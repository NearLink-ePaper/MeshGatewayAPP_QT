/**
 * @file  jpeg_decoder.c
 * @brief JPEG 解码 + Floyd-Steinberg 6色抖动 → 4bpp 墨水屏输出
 *
 * @details
 *   两阶段处理:
 *     阶段1: TJpgDec 逐 MCU 解码 JPEG → RGB888 像素写入临时行缓冲
 *     阶段2: 逐行 Floyd-Steinberg 误差扩散 → 6色量化 → 4bpp packed 输出
 *
 *   为了在有限 RAM 中工作，采用"逐 MCU 行"策略:
 *     - MCU 行高度 = 8 或 16 像素 (取决于色度子采样)
 *     - 每攒满一个 MCU 行就对其做 Floyd-Steinberg 抖动并写入 4bpp 输出
 *     - 误差缓冲区只保留当前行和下一行
 */

#include "jpeg_decoder.h"
#include "tjpgd.h"
#include "osal_debug.h"
#include "osal_addr.h"
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
    {   0,   0, 255 },  /* 5→idx4: Blue   */
    {   0, 255,   0 },  /* 6→idx5: Green  */
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

/* ── 解码上下文 ────────────────────────────────────────── */
typedef struct {
    uint8_t  *out_4bpp;     /* 4bpp 输出缓冲区 */
    uint32_t  out_cap;
    uint16_t  img_w;
    uint16_t  img_h;
    /* Floyd-Steinberg 误差缓冲: 两行，每像素 3 通道 (int16_t) */
    int16_t  *err_cur;      /* 当前行误差 */
    int16_t  *err_nxt;      /* 下一行误差 */
    uint16_t  last_y;       /* 上一次处理到的行号 */
    bool      ok;
} decode_ctx_t;

static inline int clamp8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

static uint8_t find_nearest_epd(int r, int g, int b)
{
    uint32_t best_dist = 0xFFFFFFFF;
    uint8_t  best_idx = 0;
    for (uint8_t i = 0; i < 6; i++) {
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
 * rgb_row: 该行 RGB888 数据 (w×3 bytes)
 */
static void dither_row(decode_ctx_t *ctx, const uint8_t *rgb_row, uint16_t y)
{
    uint16_t w = ctx->img_w;
    int16_t *ec = ctx->err_cur;
    int16_t *en = ctx->err_nxt;

    /* 清零下一行误差 */
    (void)memset_s(en, w * 3 * sizeof(int16_t), 0, w * 3 * sizeof(int16_t));

    uint32_t row_offset_4bpp = (uint32_t)y * (w / 2);
    if (row_offset_4bpp + w / 2 > ctx->out_cap) return;
    uint8_t *out_row = &ctx->out_4bpp[row_offset_4bpp];

    for (uint16_t x = 0; x < w; x++) {
        int oldR = clamp8((int)rgb_row[x*3+0] + ec[x*3+0]);
        int oldG = clamp8((int)rgb_row[x*3+1] + ec[x*3+1]);
        int oldB = clamp8((int)rgb_row[x*3+2] + ec[x*3+2]);

        uint8_t ci = find_nearest_epd(oldR, oldG, oldB);
        uint8_t epd_code = kPaletteToEpd[ci];

        /* 写入 4bpp packed (高 nibble = 偶像素, 低 nibble = 奇像素) */
        if (x & 1) {
            out_row[x / 2] |= epd_code;
        } else {
            out_row[x / 2] = (epd_code << 4);
        }

        /* 误差 */
        int errR = oldR - (int)kPalette[ci].r;
        int errG = oldG - (int)kPalette[ci].g;
        int errB = oldB - (int)kPalette[ci].b;

        /* →右 7/16 */
        if (x + 1 < w) {
            ec[(x+1)*3+0] += (int16_t)(errR * 7 / 16);
            ec[(x+1)*3+1] += (int16_t)(errG * 7 / 16);
            ec[(x+1)*3+2] += (int16_t)(errB * 7 / 16);
        }
        /* ↙左下 3/16 */
        if (y + 1 < ctx->img_h && x > 0) {
            en[(x-1)*3+0] += (int16_t)(errR * 3 / 16);
            en[(x-1)*3+1] += (int16_t)(errG * 3 / 16);
            en[(x-1)*3+2] += (int16_t)(errB * 3 / 16);
        }
        /* ↓下 5/16 */
        if (y + 1 < ctx->img_h) {
            en[x*3+0] += (int16_t)(errR * 5 / 16);
            en[x*3+1] += (int16_t)(errG * 5 / 16);
            en[x*3+2] += (int16_t)(errB * 5 / 16);
        }
        /* ↘右下 1/16 */
        if (y + 1 < ctx->img_h && x + 1 < w) {
            en[(x+1)*3+0] += (int16_t)(errR * 1 / 16);
            en[(x+1)*3+1] += (int16_t)(errG * 1 / 16);
            en[(x+1)*3+2] += (int16_t)(errB * 1 / 16);
        }
    }

    /* 交换 cur/nxt */
    int16_t *tmp = ctx->err_cur;
    ctx->err_cur = ctx->err_nxt;
    ctx->err_nxt = tmp;
}

/* ── TJpgDec MCU 输出回调 ─────────────────────────────── */
/* 全帧 RGB888 缓冲区 (由解码函数临时分配) */
static uint8_t *s_rgb_buf = NULL;
static uint16_t s_rgb_w = 0;

static int jpeg_output_func(JDEC *jd, void *bitmap, JRECT *rect)
{
    (void)jd;
    if (!s_rgb_buf) return 0;

    uint8_t *src = (uint8_t *)bitmap;
    uint16_t bw = rect->right - rect->left + 1;

    for (uint16_t y = rect->top; y <= rect->bottom; y++) {
        uint32_t dst_offset = (uint32_t)y * s_rgb_w * 3 + (uint32_t)rect->left * 3;
        uint32_t src_offset = (uint32_t)(y - rect->top) * bw * 3;
        (void)memcpy_s(&s_rgb_buf[dst_offset], bw * 3, &src[src_offset], bw * 3);
    }
    return 1;
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
    uint32_t rgb_size = 0;
    mem_stream_t stream;
    decode_ctx_t ctx;
    uint32_t err_row_size = 0;
    uint16_t y;

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

    /* 分配全帧 RGB888 缓冲区 */
    rgb_size = (uint32_t)w * h * 3;
    s_rgb_buf = (uint8_t *)osal_vmalloc(rgb_size);
    if (!s_rgb_buf) {
        osal_printk("%s RGB buf alloc failed (%lu)\r\n", JPEG_LOG, (unsigned long)rgb_size);
        goto cleanup;
    }
    s_rgb_w = w;
    (void)memset_s(s_rgb_buf, rgb_size, 0xFF, rgb_size);

    /* 解码整帧 */
    rc = jd_decomp(&jdec, jpeg_output_func, 0);
    if (rc != JDR_OK) {
        osal_printk("%s jd_decomp failed: %d\r\n", JPEG_LOG, rc);
        goto cleanup;
    }

    osal_printk("%s decode OK, starting dither\r\n", JPEG_LOG);

    /* Floyd-Steinberg 抖动: 逐行处理 */
    ctx.out_4bpp = out_4bpp;
    ctx.out_cap  = out_cap;
    ctx.img_w    = w;
    ctx.img_h    = h;
    ctx.ok       = true;
    ctx.err_cur  = NULL;
    ctx.err_nxt  = NULL;

    /* 误差缓冲区: 2 行 */
    err_row_size = (uint32_t)w * 3 * sizeof(int16_t);
    ctx.err_cur = (int16_t *)osal_vmalloc(err_row_size);
    ctx.err_nxt = (int16_t *)osal_vmalloc(err_row_size);
    if (!ctx.err_cur || !ctx.err_nxt) {
        osal_printk("%s err buf alloc failed\r\n", JPEG_LOG);
        if (ctx.err_cur) osal_vfree(ctx.err_cur);
        if (ctx.err_nxt) osal_vfree(ctx.err_nxt);
        goto cleanup;
    }
    (void)memset_s(ctx.err_cur, err_row_size, 0, err_row_size);
    (void)memset_s(ctx.err_nxt, err_row_size, 0, err_row_size);

    /* 清零输出 */
    (void)memset_s(out_4bpp, out_cap, 0x11, (uint32_t)(w/2)*h);  /* 0x11 = White|White */

    for (y = 0; y < h; y++) {
        dither_row(&ctx, &s_rgb_buf[(uint32_t)y * w * 3], y);
    }

    osal_vfree(ctx.err_cur);
    osal_vfree(ctx.err_nxt);

    *out_width  = w;
    *out_height = h;
    result = true;
    osal_printk("%s dither done: %dx%d → 4bpp %lu bytes\r\n",
                JPEG_LOG, w, h, (unsigned long)((uint32_t)(w/2)*h));

cleanup:
    if (s_rgb_buf) { osal_vfree(s_rgb_buf); s_rgb_buf = NULL; }
    if (pool) osal_vfree(pool);
    return result;
}
