/*
 * Native Nano-X display for Thomas Munro's ascii-invaders.
 * Copyright (c) 2026 Alex Moir and contributors.
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * The existing game updates a small character grid. Compare it once per
 * tick with the last displayed grid and draw only changed horizontal runs.
 * No PTY, ANSI parsing, per-character graphics calls, or server-side pixmap.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "nano-X.h"
#include "nxinvaders.h"

#define WIDTH 80
#define HEIGHT 25
int LINES = HEIGHT, COLS = WIDTH;
static char cells[HEIGHT][WIDTH];
static char shown[HEIGHT][WIDTH];
static int cursor_x, cursor_y;
static int repaint = 1;
static GR_WINDOW_ID window;
static GR_GC_ID gc;
static GR_COORD font_w, font_h;

void *initscr(void)
{
    GR_SCREEN_INFO screen;
    GR_FONT_INFO font;
    GR_FONT_ID fid;
    if (GrOpen() < 0) return NULL;
    GrGetScreenInfo(&screen);
    fid = GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL);
    GrGetFontInfo(fid, &font);
    font_w = font.maxwidth;
    font_h = font.height;
    window = GrNewWindowEx(GR_WM_PROPS_APPWINDOW | GR_WM_PROPS_NORESIZE,
        "Space Invaders", GR_ROOT_WINDOW_ID,
        (screen.cols - WIDTH * font_w) / 2, 25,
        WIDTH * font_w, HEIGHT * font_h, GR_RGB(0,0,0));
    gc = GrNewGC();
    GrSetGCFont(gc, fid);
    GrSetGCForeground(gc, GR_RGB(255,255,255));
    GrSetGCBackground(gc, GR_RGB(0,0,0));
    GrSetGCUseBackground(gc, GR_TRUE);
    GrSelectEvents(window, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_KEY_DOWN |
        GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_CLOSE_REQ);
    memset(cells, ' ', sizeof(cells));
    GrMapWindow(window);
    return cells;
}

void endwin(void)
{
    GrClose();
}

void move(int y, int x)
{
    cursor_y = y;
    cursor_x = x;
}

int addch(int ch)
{
    if (ch == '\n') {
        ++cursor_y;
        cursor_x = 0;
    } else {
        if (cursor_y >= 0 && cursor_y < HEIGHT &&
            cursor_x >= 0 && cursor_x < WIDTH)
            cells[cursor_y][cursor_x] = (char)ch;
        ++cursor_x;
    }
    return 0;
}

int mvaddch(int y, int x, int ch)
{
    move(y, x);
    return addch(ch);
}

static void print_format(const char *fmt, va_list args)
{
    char buf[160];
    int i;
    vsnprintf(buf, sizeof(buf), fmt, args);
    for (i = 0; buf[i]; ++i) addch((unsigned char)buf[i]);
}

void printw(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    print_format(fmt, args);
    va_end(args);
}

void mvprintw(int y, int x, const char *fmt, ...)
{
    va_list args;
    move(y, x);
    va_start(args, fmt);
    print_format(fmt, args);
    va_end(args);
}

void erase(void)
{
    memset(cells, ' ', sizeof(cells));
    move(0, 0);
}

void clrtoeol(void)
{
    int x = cursor_x < 0 ? 0 : cursor_x;
    if (cursor_y >= 0 && cursor_y < HEIGHT && x < WIDTH)
        memset(&cells[cursor_y][x], ' ', WIDTH - x);
}

/* The game calls refresh repeatedly during a tick. Present only once,
 * after both game simulation and input have updated the logical grid. */
void refresh(void) {}

void nxinvaders_present(void)
{
    int x, y, first;
    for (y = 0; y < HEIGHT; ++y) {
        x = 0;
        while (x < WIDTH) {
            if (!repaint && cells[y][x] == shown[y][x]) {
                ++x;
                continue;
            }
            first = x++;
            while (x < WIDTH && (repaint || cells[y][x] != shown[y][x])) ++x;
            GrText(window, gc, first * font_w, y * font_h,
                &cells[y][first], x - first, GR_TFASCII | GR_TFTOP);
            memcpy(&shown[y][first], &cells[y][first], x - first);
        }
    }
    repaint = 0;
    GrFlush();
}

int getch(void)
{
    GR_EVENT ev;
    GR_WINDOW_INFO info;
    int count;
    /* Bound the work per tick; the game already drains excess key events. */
    for (count = 0; count < 16; ++count) {
        GrGetNextEventTimeout(&ev, GR_TIMEOUT_POLL);
        switch (ev.type) {
        case GR_EVENT_TYPE_NONE:
        case GR_EVENT_TYPE_TIMEOUT:
            return ERR;
        case GR_EVENT_TYPE_CLOSE_REQ:
            endwin();
            exit(0);
        case GR_EVENT_TYPE_EXPOSURE:
            repaint = 1;
            break;
        case GR_EVENT_TYPE_BUTTON_DOWN:
            GrGetWindowInfo(window, &info);
            GrRaiseWindow(info.parent && info.parent != GR_ROOT_WINDOW_ID
                          ? info.parent : window);
            GrSetFocus(window);
            break;
        case GR_EVENT_TYPE_KEY_DOWN:
            if (ev.keystroke.ch == MWKEY_LEFT) return KEY_LEFT;
            if (ev.keystroke.ch == MWKEY_RIGHT) return KEY_RIGHT;
            if (ev.keystroke.ch == MWKEY_ESCAPE) {
                endwin();
                exit(0);
            }
            if (ev.keystroke.ch < 128) return ev.keystroke.ch;
            break;
        }
    }
    return ERR;
}
