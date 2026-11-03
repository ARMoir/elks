/* CGA/Tandy 1000 video memory driver for ELKS.
 * 640x200x2 and 320x200x4: two interleaved 8 KiB banks.
 * Tandy 320x200x16: four interleaved 8 KiB banks.
 * In 16-color mode pixels are two per byte, high nibble first.
 * BIOS configures the Tandy CRT/CPU video page at B8000.
 *
 * Based on Microwindows / BOGL CGA video driver.
 */
#include <assert.h>
#include "device.h"
#include "vgaplan4.h"
#include "fb.h"
#include "genmem.h"

static unsigned int bytes_per_line;
static unsigned char bank_shift, pixel_shift, bits_per_pixel, pixels_per_byte;

/* The screen stays in B800:0000 ... B800:7FFF for every supported mode. */
static FARADDR
pixel_addr(MWCOORD x, MWCOORD y)
{
    unsigned int bankmask = (1U << bank_shift) - 1U;
    unsigned int offset = (((unsigned int)y & bankmask) << 13)
         + (((unsigned int)y >> bank_shift) * bytes_per_line)
         + ((unsigned int)x >> pixel_shift);
    return MK_FP(0xb800, offset);
}

int
cga_init(PSD psd)
{
    if (psd->ncolors == 16) {
        bank_shift = 2;
        pixel_shift = 1;
        bits_per_pixel = 4;
        bytes_per_line = 160;
    } else if (psd->ncolors == 4) {
        bank_shift = 1;
        pixel_shift = 2;
        bits_per_pixel = 2;
        bytes_per_line = 80;
    } else {
        bank_shift = 1;
        pixel_shift = 3;
        bits_per_pixel = 1;
        bytes_per_line = 80;
    }
    pixels_per_byte = 1U << pixel_shift;
    psd->pitch = bytes_per_line;
    psd->addr = 0;
    psd->size = 0;
    return 1;
}

void
cga_drawpixel(PSD psd, MWCOORD x, MWCOORD y, MWPIXELVAL c)
{
    FARADDR addr;
    unsigned int shift, mask, value;
    if (x < 0 || y < 0 || x >= psd->xres || y >= psd->yres)
        return;
    shift = (pixels_per_byte - 1U - ((unsigned int)x & (pixels_per_byte - 1U))) * bits_per_pixel;
    mask = ((unsigned int)psd->ncolors - 1U) << shift;
    addr = pixel_addr(x, y);
    value = GETBYTE_FP(addr);
    if (gr_mode == MWROP_XOR)
        value ^= (((unsigned int)c << shift) & mask);
    else
        value = (value & ~mask) | (((unsigned int)c << shift) & mask);
    PUTBYTE_FP(addr, (unsigned char)value);
}

MWPIXELVAL
cga_readpixel(PSD psd, MWCOORD x, MWCOORD y)
{
    unsigned int shift;
    if (x < 0 || y < 0 || x >= psd->xres || y >= psd->yres)
        return 0;
    shift = (pixels_per_byte - 1U - ((unsigned int)x & (pixels_per_byte - 1U))) * bits_per_pixel;
    return (GETBYTE_FP(pixel_addr(x, y)) >> shift) & (psd->ncolors - 1U);
}

/* Horizontal lines include their final pixel, matching ELKS' mempl4 code.
 * Whole bytes are written together for reasonable V20 fill performance.
 */
void
cga_drawhorzline(PSD psd, MWCOORD x1, MWCOORD x2, MWCOORD y, MWPIXELVAL c)
{
    FARADDR addr;
    unsigned int fill, pixel;
    if (y < 0 || y >= psd->yres || x1 > x2 || x2 < 0 || x1 >= psd->xres)
        return;
    if (x1 < 0) x1 = 0;
    if (x2 >= psd->xres) x2 = psd->xres - 1;
    if (gr_mode != MWROP_COPY) {
        while (x1 <= x2) cga_drawpixel(psd, x1++, y, c);
        return;
    }

    /* Align the leading partial byte. */
    while (x1 <= x2 && ((unsigned int)x1 & (pixels_per_byte - 1U)))
        cga_drawpixel(psd, x1++, y, c);
    pixel = (unsigned int)c & (psd->ncolors - 1U);
    fill = pixel;
    if (bits_per_pixel == 1) fill = pixel ? 0xff : 0;
    else if (bits_per_pixel == 2) fill = pixel * 0x55U;
    else fill = pixel * 0x11U;
    if (x1 + pixels_per_byte - 1 <= x2) {
        addr = pixel_addr(x1, y);
        do {
            PUTBYTE_FP(addr, (unsigned char)fill);
            ++addr;
            x1 += pixels_per_byte;
        } while (x1 + pixels_per_byte - 1 <= x2);
    }
    while (x1 <= x2) cga_drawpixel(psd, x1++, y, c);
}

void
cga_drawvertline(PSD psd, MWCOORD x, MWCOORD y1, MWCOORD y2, MWPIXELVAL c)
{
    if (x < 0 || x >= psd->xres || y1 > y2)
        return;
    if (y1 < 0) y1 = 0;
    if (y2 >= psd->yres) y2 = psd->yres - 1;
    while (y1 <= y2) cga_drawpixel(psd, x, y1++, c);
}

static SUBDRIVER cgaplan4_none = {
    cga_drawpixel,
    cga_readpixel,
    cga_drawhorzline,
    cga_drawvertline,
    gen_fillrect,
    vga_blit,
    NULL,       /* FrameBlit*/
    NULL,       /* FrameStretchBlit*/
    0, //linear4_convblit_copy_mask_mono_byte_msb,
    NULL,       /* BlitCopyMaskMonoByteLSB*/
    NULL,       /* BlitCopyMaskMonoWordMSB*/
    NULL,       /* BlitBlendMaskAlphaByte*/
    NULL,       /* BlitCopyRGBA8888*/
    NULL,       /* BlitSrcOverRGBA8888*/
    NULL        /* BlitCopyRGB888*/

};

PSUBDRIVER cgaplan4[4] = {
    &cgaplan4_none,
#if MW_FEATURE_PORTRAIT
    &fbportrait_left, &fbportrait_right, &fbportrait_down
#endif
};
