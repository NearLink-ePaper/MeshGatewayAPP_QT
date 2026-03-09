/*----------------------------------------------------------------------------/
/  TJpgDec - Tiny JPEG Decompressor R0.03                     (C)ChaN, 2021
/  https://github.com/nickvdp/tjpgdec (Public Domain / Unlicense)
/
/  Adapted for SLE Mesh e-Paper project:
/    - JD_SZBUF = 512  (input stream buffer)
/    - JD_FORMAT = 0    (RGB888 output)
/    - JD_FASTDECODE = 1
/-----------------------------------------------------------------------------*/
#ifndef TJPGDEC_H
#define TJPGDEC_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration ------------------------------------------------------------ */
#define JD_SZBUF       512   /* Size of stream input buffer */
#define JD_FORMAT       0    /* 0: RGB888, 1: RGB565 */
#define JD_USE_SCALE    1    /* Enable output scaling */
#define JD_TBLCLIP      1    /* Use clipping table for saturation */
#define JD_FASTDECODE   1    /* Fast decode mode */

/* Error codes -------------------------------------------------------------- */
typedef enum {
    JDR_OK = 0,     /* 0: Succeeded */
    JDR_INTR,       /* 1: Interrupted by output function */
    JDR_INP,        /* 2: Device error or wrong termination of input stream */
    JDR_MEM1,       /* 3: Insufficient memory pool for the image */
    JDR_MEM2,       /* 4: Insufficient stream input buffer */
    JDR_PAR,        /* 5: Parameter error */
    JDR_FMT1,       /* 6: Data format error (may be broken data) */
    JDR_FMT2,       /* 7: Right format but not supported */
    JDR_FMT3        /* 8: Not supported JPEG standard */
} JRESULT;

/* Rectangular region in the output image ----------------------------------- */
typedef struct {
    uint16_t left;
    uint16_t right;
    uint16_t top;
    uint16_t bottom;
} JRECT;

/* Decompressor object structure -------------------------------------------- */
typedef struct JDEC JDEC;
struct JDEC {
    unsigned int dctr;          /* Number of bytes available in the input buffer */
    uint8_t *dptr;              /* Current data read ptr */
    uint8_t *inbuf;             /* Bit stream input buffer */
    uint8_t dbit;               /* Number of bits availavle in wreg or reading bit */
    uint8_t scale;              /* Output scaling ratio */
    uint8_t msx, msy;           /* MCU size in unit of block (width, height) */
    uint8_t qtid[3];            /* Quantization table ID of each component */
    uint8_t ncomp;              /* Number of color components 1:grayscale, 3:color */
    int16_t dcv[3];             /* Previous DC element of each component */
    uint16_t nrst;              /* Restart interval */
    unsigned int width, height; /* Size of the input image (pixel) */
    uint8_t *huffbits[2][2];    /* Huffman bit distribution tables [id][dcac] */
    uint16_t *huffcode[2][2];   /* Huffman code word tables [id][dcac] */
    uint8_t *huffdata[2][2];    /* Huffman decoded data tables [id][dcac] */
    int32_t *qttbl[4];         /* Dequantizer tables [id] */
#if JD_FASTDECODE >= 1
    uint32_t wreg;              /* Working shift register */
    uint8_t marker;             /* Detected marker (0:None) */
#if JD_FASTDECODE == 2
    uint8_t longofs[2][2];
    uint16_t *hufflut_ac[2];
    uint8_t *hufflut_dc[2];
#endif
#endif
    void *workbuf;              /* Working buffer for IDCT and target pixel extraction */
    uint8_t *mcubuf;            /* Working buffer for MCU */
    void *pool;                 /* Pointer to available memory pool */
    unsigned int sz_pool;       /* Size of memory pool (bytes available) */
    unsigned int (*infunc)(JDEC*, uint8_t*, unsigned int); /* Stream input function */
    void *device;               /* Pointer to I/O device identifier for the session */
};

/* TJpgDec API functions ---------------------------------------------------- */

/**
 * @brief Analyze a JPEG data and create a decompressor object for subsequent decompression
 * @param jd     Pointer to blank decompressor object
 * @param infunc Data input function
 * @param pool   Pointer to memory pool for the decompressor
 * @param sz_pool Size of memory pool (bytes), recommended >= 3500
 * @param dev    Pointer to user-defined device identifier
 */
JRESULT jd_prepare(JDEC *jd, unsigned int (*infunc)(JDEC*,uint8_t*,unsigned int),
                   void *pool, unsigned int sz_pool, void *dev);

/**
 * @brief Decompress the JPEG image and output via callback
 * @param jd     Decompressor object (prepared by jd_prepare)
 * @param outfunc Data output function (called per MCU block)
 * @param scale  Output de-scaling factor (0:1/1, 1:1/2, 2:1/4, 3:1/8)
 */
JRESULT jd_decomp(JDEC *jd, int (*outfunc)(JDEC*,void*,JRECT*), uint8_t scale);

#ifdef __cplusplus
}
#endif

#endif /* TJPGDEC_H */
