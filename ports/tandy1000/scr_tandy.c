/* Tandy 1000 EX multi-mode screen driver. Based on Microwindows scr_cga.c. */
/*
 * CGA Screen Driver for ELKS
 * 	For CGA, 2 color, 640x200 resolution
 * 	This driver uses bios for graphical setting.
 *
 *      This driver is created and modifed, based on EGA/VGA driver.
 * 	This file itself doesn't know about any planar or packed arrangement, relying
 * 	on routines in vgaplan4_cga.c for drawing.
 *
 *      Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 *
 * T. Yamada 2024
 */

#include <linuxmt/ntty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device.h"
#include "vgaplan4.h"
#include "genmem.h"
#if ROMFONT
#include "romfont.h"
#else
#include "genfont.h"
#endif

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

/* CGA driver entry points*/
static PSD  CGA_open(PSD psd);
static void CGA_close(PSD psd);
static void CGA_getscreeninfo(PSD psd, PMWSCREENINFO psi);;
static void CGA_setpalette(PSD psd,int first,int count, MWPALENTRY *pal);

SCREENDEVICE	scrdev = {
    0, 0, 0, 0, 0, 0, 0, NULL, 0, NULL, 0, 0, 0, 0, 0, 0,
#if ROMFONT
        pcrom_fonts,
#else
        gen_fonts,
#endif
        CGA_open,
        CGA_close,
        CGA_setpalette,
        CGA_getscreeninfo,
        gen_allocatememgc,
        gen_mapmemgc,
        gen_freememgc,
        0,                      /* gen_setportrait*/
        NULL,                   /* PreSelect*/
};

/* ELKS Tandy/CGA screen selection, independent of serial mouse selection.
 * BIOS mode 06h: 640x200 mono, 04h: 320x200 4-color CGA,
 * 09h: 320x200 16-color Tandy 1000.
 */
static int graphics_mode = 2;
#define FNTEXT 0x0003

static PSD
CGA_open(PSD psd)
{
    const char *choice = getenv("TANDY_MODE");
    extern PSUBDRIVER cgaplan4[4];

    if (choice && strcmp(choice, "mono") == 0)
        graphics_mode = 0;
    else if (choice && strcmp(choice, "color") == 0)
        graphics_mode = 1;
    else if (choice && strcmp(choice, "tandy") == 0)
        graphics_mode = 2;
    else if (getenv("MONOMODE"))
        graphics_mode = 0;
    else
        graphics_mode = 2;

    if (graphics_mode == 2)
        int10(0x0009, 0);      /* Tandy 320x200, 16 colors, 32 KiB */
    else if (graphics_mode == 1) {
        int10(0x0004, 0);      /* CGA 320x200, 4 colors */
        int10(0x0b00, 0x0101);/* CGA cyan/magenta/white palette */
    } else
        int10(0x0006, 0);      /* CGA 640x200, 2 colors */

    psd->xres = psd->xvirtres = graphics_mode == 0 ? 640 : 320;
    psd->yres = psd->yvirtres = 200;
    psd->planes = 1;
    /* ELKS memory surfaces use packed 4bpp regardless of video bit depth. */
    psd->bpp = 4;
    psd->ncolors = graphics_mode == 0 ? 2 : graphics_mode == 1 ? 4 : 16;
    psd->pixtype = MWPF_PALETTE;
    psd->flags = PSF_SCREEN;
    set_subdriver(psd, cgaplan4[0]);
    cga_init(psd);
#if ROMFONT
    pcrom_init(psd);
#endif
    return psd;
}

static void
CGA_close(PSD psd)
{
	int10(FNTEXT, 0);           /* init bios 80x25 text mode*/
}

static void
CGA_getscreeninfo(PSD psd, PMWSCREENINFO psi)
{
	psi->rows = psd->yvirtres;
	psi->cols = psd->xvirtres;
	psi->planes = psd->planes;
	psi->bpp = psd->bpp;
    psd->data_format = psd->data_format;
	psi->ncolors = psd->ncolors;
	psi->pixtype = psd->pixtype;
	psi->fonts = NUMBER_FONTS;

	/* Approximate physical screen width of 24 cm */
	psi->xdpcm = graphics_mode == 0 ? 27 : 13;	/* assumes screen width of 24 cm*/
	psi->ydpcm = 11;	/* assumes screen height of 18 cm*/
}

static void
CGA_setpalette(PSD psd,int first,int count, MWPALENTRY *pal)
{
	/* not yet implemented, std 16 color palette assumed*/
}
