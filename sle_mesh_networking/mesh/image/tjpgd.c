/*----------------------------------------------------------------------------/
/  TJpgDec - Tiny JPEG Decompressor R0.03        (C)ChaN, 2021
/  http://elm-chan.org/fsw/tjpgd/00index.html
/
/  This is a free software and is opened for education, research and commercial
/  developments under license policy of following terms.
/
/  Copyright (C) 2021, ChaN, all right reserved.
/
/  The TJpgDec module is a free software and there is NO WARRANTY.
/  No restriction on use. You can use, modify and redistribute it for
/  personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/  Redistributions of source code must retain the above copyright notice.
/
/  Adapted for SLE Mesh e-Paper project — minimal footprint build.
/-----------------------------------------------------------------------------*/

#include "tjpgd.h"

#if JD_TBLCLIP
/* Clipping table for saturation arithmetic (0-255) */
static const uint8_t Clip8[1024] = {
    /* 0 - 255: identity */
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
    64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
    96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
    128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
    160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
    192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
    224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
    /* 256 - 511: clip to 255 */
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    /* 512 - 767: padding 0 (for negative index via BYTECLIP) */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 768 - 1023: padding 0 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
#define BYTECLIP(v) Clip8[(unsigned int)(v) & 0x3FF]
#else
static inline uint8_t BYTECLIP(int v) { return (v < 0) ? 0 : (v > 255) ? 255 : (uint8_t)v; }
#endif

/* Zig-zag scan order */
static const uint8_t Zig[64] = {
     0, 1, 8,16, 9, 2, 3,10,17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34,27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
};

/* IDCT coefficients */
#define ICOS(n) ((int32_t)Icos[n])
static const int16_t Icos[7] = { 16384, 22725, 21407, 19266, 16384, 12873, 8867 };

/*-----------------------------------------------------------------------*/
/* Memory-buffer based input stream                                      */
/*-----------------------------------------------------------------------*/

typedef struct {
    const uint8_t *data;
    unsigned int   size;
    unsigned int   pos;
} mem_stream_t;

/*-----------------------------------------------------------------------*/
/* Allocate from memory pool                                             */
/*-----------------------------------------------------------------------*/
static void *alloc_pool(JDEC *jd, unsigned int nd)
{
    char *rp = (char *)jd->pool;
    nd = (nd + 3) & ~3;  /* align to 4 bytes */
    if (jd->sz_pool < nd) return 0;
    jd->sz_pool -= nd;
    jd->pool = rp + nd;
    return rp;
}

/*-----------------------------------------------------------------------*/
/* Create de-quantized and prescaled DCT coefficient array               */
/*-----------------------------------------------------------------------*/
static int create_qt_tbl(JDEC *jd, const uint8_t *data, unsigned int ndata)
{
    unsigned int i;
    uint8_t d;
    int32_t *tbl;

    while (ndata) {
        if (ndata < 65) return JDR_FMT1;
        d = *data++;
        ndata--;
        if (d & 0xF0) return JDR_FMT1;  /* 16-bit precision not supported */
        i = d & 0x03;
        tbl = (int32_t *)alloc_pool(jd, 64 * sizeof(int32_t));
        if (!tbl) return JDR_MEM1;
        jd->qttbl[i] = tbl;
        for (i = 0; i < 64; i++) {
            tbl[i] = (int32_t)data[i] * Icos[Zig[i] & 7] * Icos[(Zig[i] >> 3) & 7] >> 5;
        }
        data += 64;
        ndata -= 64;
    }
    return JDR_OK;
}

/*-----------------------------------------------------------------------*/
/* Create huffman decode table                                           */
/*-----------------------------------------------------------------------*/
static int create_huffman_tbl(JDEC *jd, const uint8_t *data, unsigned int ndata)
{
    unsigned int i, j, b, cls, num;
    uint8_t d, *pb, *pd;
    uint16_t hc, *pc;

    while (ndata) {
        if (ndata < 17) return JDR_FMT1;
        d = *data++;
        ndata--;
        cls = (d >> 4); num = d & 0x0F;
        if (cls > 1 || num > 1) return JDR_FMT1;

        /* Allocate bit distribution table */
        pb = (uint8_t *)alloc_pool(jd, 16);
        if (!pb) return JDR_MEM1;
        jd->huffbits[num][cls] = pb;
        for (i = b = 0; i < 16; i++) {
            pb[i] = data[i];
            b += pb[i];
        }
        data += 16;
        ndata -= 16;

        /* Create code word table */
        pc = (uint16_t *)alloc_pool(jd, b * 2);
        if (!pc) return JDR_MEM1;
        jd->huffcode[num][cls] = pc;
        for (i = hc = 0; i < 16; i++) {
            for (j = 0; j < pb[i]; j++) {
                *pc++ = hc++;
            }
            hc <<= 1;
        }

        /* Create decoded data table */
        if (ndata < b) return JDR_FMT1;
        pd = (uint8_t *)alloc_pool(jd, b);
        if (!pd) return JDR_MEM1;
        jd->huffdata[num][cls] = pd;
        for (i = 0; i < b; i++) {
            pd[i] = data[i];
        }
        data += b;
        ndata -= b;
    }
    return JDR_OK;
}

/*-----------------------------------------------------------------------*/
/* Read bits from input stream                                           */
/*-----------------------------------------------------------------------*/
static int huffext(JDEC *jd, unsigned int id, unsigned int cls)
{
    unsigned int dc, num, wbit, code;
    uint8_t *hb, *hd;
    uint16_t *hc;
    const unsigned int msk[17] = {
        0x0000,0x0001,0x0003,0x0007,0x000F,0x001F,0x003F,0x007F,
        0x00FF,0x01FF,0x03FF,0x07FF,0x0FFF,0x1FFF,0x3FFF,0x7FFF,0xFFFF
    };

    hb = jd->huffbits[id][cls];
    hc = jd->huffcode[id][cls];
    hd = jd->huffdata[id][cls];
    if (!hb || !hc || !hd) return -(int)JDR_FMT1;

    num = 0;
    code = 0;
    wbit = jd->dbit % 8;

    for (dc = 1; dc <= 16; dc++) {
        /* Need a bit */
        if (!wbit) {
            /* Get next byte */
            if (!jd->dctr) {
                /* Refill input buffer */
                unsigned int nd = jd->infunc(jd, 0, JD_SZBUF);
                if (!nd) return -(int)JDR_INP;
                jd->dctr = nd;
                jd->dptr = jd->inbuf;
            }
            uint8_t s = *jd->dptr++;
            jd->dctr--;
#if JD_FASTDECODE >= 1
            if (s == 0xFF) {
                /* Skip stuff byte */
                if (!jd->dctr) {
                    unsigned int nd = jd->infunc(jd, 0, JD_SZBUF);
                    if (!nd) return -(int)JDR_INP;
                    jd->dctr = nd;
                    jd->dptr = jd->inbuf;
                }
                uint8_t m = *jd->dptr;
                if (m != 0) {
                    jd->marker = m;
                    jd->dptr++;
                    jd->dctr--;
                }
                /* If m == 0, it's a stuffed 0xFF byte — keep s as 0xFF */
                else {
                    jd->dptr++;
                    jd->dctr--;
                }
            }
#endif
            jd->wreg = (jd->wreg << 8) | s;
            wbit = 8;
        }

        wbit--;
        code = (code << 1) | ((jd->wreg >> wbit) & 1);

        for (unsigned int i = 0; i < hb[dc - 1]; i++) {
            if (hc[num] == code) {
                jd->dbit = (jd->dbit & ~7) | wbit;
                return hd[num];
            }
            num++;
        }
    }

    return -(int)JDR_FMT1;
}

/*-----------------------------------------------------------------------*/
/* Extract N bits from input stream                                      */
/*-----------------------------------------------------------------------*/
static int bitext(JDEC *jd, unsigned int nbit)
{
    uint32_t wbit = jd->dbit % 8;
    int32_t d = 0;

    while (nbit) {
        if (!wbit) {
            if (!jd->dctr) {
                unsigned int nd = jd->infunc(jd, 0, JD_SZBUF);
                if (!nd) return 0;
                jd->dctr = nd;
                jd->dptr = jd->inbuf;
            }
            uint8_t s = *jd->dptr++;
            jd->dctr--;
#if JD_FASTDECODE >= 1
            if (s == 0xFF) {
                if (!jd->dctr) {
                    unsigned int nd = jd->infunc(jd, 0, JD_SZBUF);
                    if (!nd) return 0;
                    jd->dctr = nd;
                    jd->dptr = jd->inbuf;
                }
                uint8_t m = *jd->dptr;
                if (m != 0) {
                    jd->marker = m;
                    jd->dptr++;
                    jd->dctr--;
                } else {
                    jd->dptr++;
                    jd->dctr--;
                }
            }
#endif
            jd->wreg = (jd->wreg << 8) | s;
            wbit = 8;
        }
        unsigned int take = (nbit < wbit) ? nbit : wbit;
        wbit -= take;
        nbit -= take;
        d = (d << take) | ((jd->wreg >> wbit) & ((1U << take) - 1));
    }

    jd->dbit = (jd->dbit & ~7) | wbit;
    return d;
}

/*-----------------------------------------------------------------------*/
/* Restart marker handling                                               */
/*-----------------------------------------------------------------------*/
static JRESULT restart(JDEC *jd, uint16_t rstn)
{
    unsigned int i;
    uint8_t *dp = jd->dptr;
    unsigned int dc = jd->dctr;

#if JD_FASTDECODE >= 1
    /* If we saw a marker during bit reads, check for restart marker */
    if (jd->marker) {
        uint8_t m = jd->marker;
        jd->marker = 0;
        if (m != (0xD0 | (rstn & 7))) return JDR_FMT1;
    } else
#endif
    {
        /* Read forward to find the RST marker */
        for (i = 0; i < 4; i++) {
            if (!dc) {
                unsigned int nd = jd->infunc(jd, 0, JD_SZBUF);
                if (!nd) return JDR_INP;
                dp = jd->inbuf;
                dc = nd;
            }
            uint8_t b = *dp++;
            dc--;
            if (b == 0xFF) {
                if (!dc) {
                    unsigned int nd = jd->infunc(jd, 0, JD_SZBUF);
                    if (!nd) return JDR_INP;
                    dp = jd->inbuf;
                    dc = nd;
                }
                b = *dp++;
                dc--;
                if (b == (0xD0 | (rstn & 7))) break;
                if (b == 0) continue; /* stuffed FF */
                return JDR_FMT1;
            }
        }
    }

    jd->dptr = dp;
    jd->dctr = dc;
    jd->dbit = 0;
#if JD_FASTDECODE >= 1
    jd->wreg = 0;
#endif
    /* Reset DC values */
    jd->dcv[0] = jd->dcv[1] = jd->dcv[2] = 0;
    return JDR_OK;
}

/*-----------------------------------------------------------------------*/
/* Apply IDCT and store pixel block                                      */
/*-----------------------------------------------------------------------*/
static void block_idct(int32_t *src, uint8_t *dst)
{
    int32_t v0, v1, v2, v3, v4, v5, v6, v7;
    int32_t t10, t11, t12, t13;
    int32_t *sptr;
    uint8_t *dptr;
    int i;

    /* Process rows */
    sptr = src;
    for (i = 0; i < 8; i++) {
        v0 = sptr[0]; v1 = sptr[1]; v2 = sptr[2]; v3 = sptr[3];
        v4 = sptr[4]; v5 = sptr[5]; v6 = sptr[6]; v7 = sptr[7];

        if (!(v1|v2|v3|v4|v5|v6|v7)) {
            v0 <<= 2;
            sptr[0]=sptr[1]=sptr[2]=sptr[3]=sptr[4]=sptr[5]=sptr[6]=sptr[7]=v0;
        } else {
            t10 = v0 + v4; t12 = v0 - v4;
            t11 = (v2 * ICOS(2) - v6 * ICOS(6)) >> 14;
            t13 = (v2 * ICOS(6) + v6 * ICOS(2)) >> 14;
            v0 = t10 + t13; v3 = t10 - t13;
            v1 = t12 + t11; v2 = t12 - t11;

            t10 = (v5 * ICOS(2) - v3 * ICOS(6)) >> 14;
            t12 = (v5 * ICOS(6) + v3 * ICOS(2)) >> 14;
            t11 = v1 * ICOS(4) >> 14;
            t13 = v7 * ICOS(4) >> 14;

            /* Simplified butterfly */
            v4 = t11 + t10; v7 = t13 + t12;
            v5 = t11 - t10; v6 = t13 - t12;

            sptr[0] = v0 + v7; sptr[7] = v0 - v7;
            sptr[1] = v1 + v6; sptr[6] = v1 - v6;
            sptr[2] = v2 + v5; sptr[5] = v2 - v5;
            sptr[3] = v3 + v4; sptr[4] = v3 - v4;
        }
        sptr += 8;
    }

    /* Process columns and output */
    dptr = dst;
    for (i = 0; i < 8; i++) {
        v0=src[i]; v1=src[8+i]; v2=src[16+i]; v3=src[24+i];
        v4=src[32+i]; v5=src[40+i]; v6=src[48+i]; v7=src[56+i];

        if (!(v1|v2|v3|v4|v5|v6|v7)) {
            uint8_t c = BYTECLIP(((v0 >> 4) + 128));
            dptr[i]=dptr[8+i]=dptr[16+i]=dptr[24+i]=
            dptr[32+i]=dptr[40+i]=dptr[48+i]=dptr[56+i]= c;
        } else {
            t10 = v0 + v4; t12 = v0 - v4;
            t11 = (v2 * ICOS(2) - v6 * ICOS(6)) >> 14;
            t13 = (v2 * ICOS(6) + v6 * ICOS(2)) >> 14;
            v0 = t10 + t13; v3 = t10 - t13;
            v1 = t12 + t11; v2 = t12 - t11;

            t10 = (v5 * ICOS(2) - v3 * ICOS(6)) >> 14;
            t12 = (v5 * ICOS(6) + v3 * ICOS(2)) >> 14;
            t11 = v1 * ICOS(4) >> 14;
            t13 = v7 * ICOS(4) >> 14;

            v4 = t11 + t10; v7 = t13 + t12;
            v5 = t11 - t10; v6 = t13 - t12;

            dptr[i]    = BYTECLIP(((v0 + v7) >> 4) + 128);
            dptr[56+i] = BYTECLIP(((v0 - v7) >> 4) + 128);
            dptr[8+i]  = BYTECLIP(((v1 + v6) >> 4) + 128);
            dptr[48+i] = BYTECLIP(((v1 - v6) >> 4) + 128);
            dptr[16+i] = BYTECLIP(((v2 + v5) >> 4) + 128);
            dptr[40+i] = BYTECLIP(((v2 - v5) >> 4) + 128);
            dptr[24+i] = BYTECLIP(((v3 + v4) >> 4) + 128);
            dptr[32+i] = BYTECLIP(((v3 - v4) >> 4) + 128);
        }
    }
}

/*-----------------------------------------------------------------------*/
/* MCU output (color conversion + block compose)                         */
/*-----------------------------------------------------------------------*/
static int mcu_output(
    JDEC *jd,
    int (*outfunc)(JDEC*, void*, JRECT*),
    unsigned int x, unsigned int y)
{
    unsigned int ix, iy, mx, my, bx, by;
    uint8_t *op, *mcb = jd->mcubuf;
    uint8_t *workbuf = (uint8_t *)jd->workbuf;
    JRECT rect;

    mx = jd->msx * 8; my = jd->msy * 8;
    unsigned int scale = 1 << jd->scale;
    mx /= scale; my /= scale;

    if (x + mx > jd->width / scale) mx = jd->width / scale - x;
    if (y + my > jd->height / scale) my = jd->height / scale - y;

    rect.left = (uint16_t)x;
    rect.right = (uint16_t)(x + mx - 1);
    rect.top = (uint16_t)y;
    rect.bottom = (uint16_t)(y + my - 1);

    if (jd->ncomp == 3) {
        /* YCbCr to RGB conversion */
        uint8_t *yp, *cbp, *crp;
        unsigned int msx8 = jd->msx * 8, msy8 = jd->msy * 8;
        op = workbuf;
        for (iy = 0; iy < my; iy++) {
            for (ix = 0; ix < mx; ix++) {
                unsigned int px = ix * scale, py = iy * scale;
                /* Y sample */
                bx = px / 8; by = py / 8;
                yp = &mcb[(by * jd->msx + bx) * 64 + (py % 8) * 8 + (px % 8)];
                /* Cb,Cr sample (subsampled) */
                unsigned int csx = px * 8 / msx8, csy = py * 8 / msy8;
                unsigned int cidx = (jd->msx * jd->msy) * 64 + csy * 8 + csx;
                cbp = &mcb[cidx];
                crp = &mcb[cidx + 64];

                int yy = (int)*yp;
                int cb = (int)*cbp - 128;
                int cr = (int)*crp - 128;
#if JD_FORMAT == 0  /* RGB888 */
                *op++ = BYTECLIP(yy + ((int)(1.402 * 16384) * cr >> 14));
                *op++ = BYTECLIP(yy - ((int)(0.344 * 16384) * cb >> 14) - ((int)(0.714 * 16384) * cr >> 14));
                *op++ = BYTECLIP(yy + ((int)(1.772 * 16384) * cb >> 14));
#endif
            }
        }
    } else {
        /* Grayscale */
        op = workbuf;
        for (iy = 0; iy < my; iy++) {
            for (ix = 0; ix < mx; ix++) {
                unsigned int px = ix * scale, py = iy * scale;
                uint8_t g = mcb[(py % 8) * 8 + (px % 8)];
#if JD_FORMAT == 0
                *op++ = g; *op++ = g; *op++ = g;
#endif
            }
        }
    }

    return outfunc(jd, workbuf, &rect);
}

/*-----------------------------------------------------------------------*/
/* Process a single MCU                                                  */
/*-----------------------------------------------------------------------*/
static JRESULT mcu_load(JDEC *jd)
{
    int32_t *tmp = (int32_t *)jd->workbuf;
    unsigned int blk, nby, ncomp = jd->ncomp;
    uint8_t *bp = jd->mcubuf;

    nby = jd->msx * jd->msy;

    for (blk = 0; blk < nby + (ncomp == 3 ? 2 : 0); blk++) {
        unsigned int cid = (blk < nby) ? 0 : blk - nby + 1;
        unsigned int qid = jd->qtid[cid];
        int32_t *qt = jd->qttbl[qid];

        memset(tmp, 0, 64 * sizeof(int32_t));

        /* DC coefficient */
        int d = huffext(jd, (cid ? 1 : 0), 0);
        if (d < 0) return (JRESULT)(-(d));
        if (d) {
            int v = bitext(jd, d);
            if (!(v & (1 << (d - 1)))) v -= (1 << d) - 1;
            jd->dcv[cid] += (int16_t)v;
        }
        tmp[0] = (int32_t)jd->dcv[cid] * qt[0] >> 8;

        /* AC coefficients */
        unsigned int i = 1;
        while (i < 64) {
            d = huffext(jd, (cid ? 1 : 0), 1);
            if (d < 0) return (JRESULT)(-(d));
            if (d == 0) break;  /* EOB */
            unsigned int z = (unsigned int)d >> 4;  /* Zero run */
            d &= 0x0F;
            if (d == 0 && z == 15) { i += 16; continue; }  /* ZRL */
            i += z;
            if (i >= 64) break;
            if (d) {
                int v = bitext(jd, d);
                if (!(v & (1 << (d - 1)))) v -= (1 << d) - 1;
                tmp[Zig[i]] = (int32_t)v * qt[i] >> 8;
            }
            i++;
        }

        /* IDCT */
        block_idct(tmp, bp);
        bp += 64;
    }
    return JDR_OK;
}

/*-----------------------------------------------------------------------*/
/* Analyze the JPEG data and create decompressor object                  */
/*-----------------------------------------------------------------------*/
JRESULT jd_prepare(JDEC *jd, unsigned int (*infunc)(JDEC*,uint8_t*,unsigned int),
                   void *pool, unsigned int sz_pool, void *dev)
{
    uint8_t *seg;
    unsigned int n, len;
    JRESULT rc;

    if (!jd || !infunc || !pool) return JDR_PAR;

    jd->pool = pool;
    jd->sz_pool = sz_pool;
    jd->infunc = infunc;
    jd->device = dev;
    jd->nrst = 0;
#if JD_FASTDECODE >= 1
    jd->wreg = 0;
    jd->marker = 0;
#endif
    jd->dcv[0] = jd->dcv[1] = jd->dcv[2] = 0;

    /* Allocate input buffer */
    jd->inbuf = (uint8_t *)alloc_pool(jd, JD_SZBUF);
    if (!jd->inbuf) return JDR_MEM1;

    /* Load initial data */
    jd->dctr = infunc(jd, jd->inbuf, JD_SZBUF);
    if (jd->dctr <= 2) return JDR_INP;
    jd->dptr = jd->inbuf;
    jd->dbit = 0;

    /* Check SOI marker */
    if (jd->dptr[0] != 0xFF || jd->dptr[1] != 0xD8) return JDR_FMT1;
    jd->dptr += 2;
    jd->dctr -= 2;

    /* Parse segments */
    for (;;) {
        /* Get next marker */
        while (jd->dctr < 4) {
            if (jd->dctr) {
                uint8_t *p = jd->inbuf;
                unsigned int r = jd->dctr;
                for (unsigned int i = 0; i < r; i++) p[i] = jd->dptr[i];
                jd->dptr = p;
            }
            n = infunc(jd, jd->inbuf + jd->dctr, JD_SZBUF - jd->dctr);
            if (!n) return JDR_INP;
            jd->dctr += n;
            jd->dptr = jd->inbuf;
        }

        uint8_t m0 = jd->dptr[0], m1 = jd->dptr[1];
        if (m0 != 0xFF) return JDR_FMT1;
        jd->dptr += 2;
        jd->dctr -= 2;

        /* SOS marker — done parsing */
        if (m1 == 0xDA) {
            /* Skip SOS header */
            len = ((unsigned int)jd->dptr[0] << 8) | jd->dptr[1];
            jd->dptr += len;
            jd->dctr -= len;
            break;
        }

        /* Get segment length */
        len = ((unsigned int)jd->dptr[0] << 8) | jd->dptr[1];
        jd->dptr += 2;
        jd->dctr -= 2;
        len -= 2;

        /* Load the segment data into pool temporarily */
        seg = (uint8_t *)alloc_pool(jd, len);
        if (!seg) return JDR_MEM1;

        unsigned int loaded = 0;
        while (loaded < len) {
            unsigned int avail = (jd->dctr < len - loaded) ? jd->dctr : len - loaded;
            memcpy(seg + loaded, jd->dptr, avail);
            jd->dptr += avail;
            jd->dctr -= avail;
            loaded += avail;
            if (loaded < len) {
                jd->dctr = infunc(jd, jd->inbuf, JD_SZBUF);
                if (!jd->dctr) return JDR_INP;
                jd->dptr = jd->inbuf;
            }
        }

        switch (m1) {
        case 0xC0:  /* SOF0 (Baseline) */
            if (len < 9) return JDR_FMT1;
            if (seg[0] != 8) return JDR_FMT3;  /* Only 8-bit precision */
            jd->height = ((unsigned int)seg[1] << 8) | seg[2];
            jd->width  = ((unsigned int)seg[3] << 8) | seg[4];
            jd->ncomp = seg[5];
            if (jd->ncomp != 1 && jd->ncomp != 3) return JDR_FMT3;
            if (jd->ncomp == 3) {
                if (len < 15) return JDR_FMT1;
                /* Component info */
                jd->msx = (seg[7] >> 4);
                jd->msy = (seg[7] & 0x0F);
                jd->qtid[0] = seg[8];
                jd->qtid[1] = seg[11];
                jd->qtid[2] = seg[14];
            } else {
                jd->msx = jd->msy = 1;
                jd->qtid[0] = seg[8];
            }
            break;
        case 0xDB:  /* DQT */
            rc = (JRESULT)create_qt_tbl(jd, seg, len);
            if (rc) return rc;
            break;
        case 0xC4:  /* DHT */
            rc = (JRESULT)create_huffman_tbl(jd, seg, len);
            if (rc) return rc;
            break;
        case 0xDD:  /* DRI */
            if (len >= 2) {
                jd->nrst = ((uint16_t)seg[0] << 8) | seg[1];
            }
            break;
        default:
            /* Skip unknown segments */
            break;
        }

        /* Return segment memory to pool by just moving pool pointer back */
        jd->sz_pool += (len + 3) & ~3;
        jd->pool = seg;
    }

    /* Allocate MCU buffer and working buffer */
    n = jd->msx * jd->msy;
    if (jd->ncomp == 3) n += 2;  /* +Cb,Cr blocks */
    jd->mcubuf = (uint8_t *)alloc_pool(jd, n * 64);
    if (!jd->mcubuf) return JDR_MEM1;
    jd->workbuf = alloc_pool(jd, (jd->msx * 8) * (jd->msy * 8) * 3);
    if (!jd->workbuf) return JDR_MEM1;

    return JDR_OK;
}

/*-----------------------------------------------------------------------*/
/* Decompress the JPEG image                                             */
/*-----------------------------------------------------------------------*/
JRESULT jd_decomp(JDEC *jd, int (*outfunc)(JDEC*,void*,JRECT*), uint8_t scale)
{
    unsigned int x, y, mx, my;
    JRESULT rc;
    uint16_t rst = 0, rstc = 0;

    if (!jd) return JDR_PAR;
    if (scale > 3) return JDR_PAR;
    jd->scale = scale;

    mx = jd->msx * 8;
    my = jd->msy * 8;

    for (y = 0; y < jd->height; y += my) {
        for (x = 0; x < jd->width; x += mx) {
            /* Restart interval processing */
            if (jd->nrst && rstc++ >= jd->nrst) {
                rc = restart(jd, rst++);
                if (rc != JDR_OK) return rc;
                rstc = 1;
            }
            /* Load MCU */
            rc = mcu_load(jd);
            if (rc != JDR_OK) return rc;

            /* Output MCU */
            unsigned int sx = x >> scale, sy = y >> scale;
            if (!mcu_output(jd, outfunc, sx, sy)) return JDR_INTR;
        }
    }

    return JDR_OK;
}
