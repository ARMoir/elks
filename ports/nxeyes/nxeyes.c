/*
 * nxeyes - tiny Nano-X eyes for the 2 MB IBM PS/2 running ELKS.
 *
 * Inspired by the XEyes demonstration, independently implemented for
 * Microwindows/Nano-X's ELKS configuration. No X11 libraries, shapes,
 * floating point, timers or special input drivers are required.
 *
 * Nano-X's ELKS build sets MW_FEATURE_TINY=1 (no GrQueryPointer).
 * GrGetScreenInfo provides the live screen-level cursor coordinates,
 * even while the cursor is above a different client window.
 *
 * Deliberately low redraw rate and no changes to the Nano-X server.
 */
#include <stdio.h>
#include <stdlib.h>
#include "nano-X.h"

#define EYES_W      94
#define EYES_H      54
#define EYE_Y       27
#define LEFT_X      24
#define RIGHT_X     70
#define EYE_RX      19
#define EYE_RY      23
#define PUPIL_RX    5
#define PUPIL_RY    6
#define PUPIL_LIMIT 10
#define POLL_MS     150L

static GR_WINDOW_ID win;
static GR_GC_ID gc;
static int drawn;
static int old_px[2] = { -100, -100 };
static int old_py[2] = { -100, -100 };

static void color(GR_COLOR c)
{
    GrSetGCForeground(gc, c);
}

static void oval(int x, int y, int rx, int ry, GR_COLOR c)
{
    color(c);
    GrFillEllipse(win, gc, x, y, rx, ry);
}

/* Scale a vector down to at most 10 pixels without libm or floating point. */
static void pupil_position(int center_x, int center_y,
                           int cursor_x, int cursor_y,
                           int *px, int *py)
{
    int dx = cursor_x - center_x;
    int dy = cursor_y - center_y;
    int ax = dx < 0 ? -dx : dx;
    int ay = dy < 0 ? -dy : dy;
    int scale = ax > ay ? ax : ay;

    if (scale > PUPIL_LIMIT) {
        dx = (dx * PUPIL_LIMIT) / scale;
        dy = (dy * PUPIL_LIMIT) / scale;
    }
    *px = center_x + dx;
    *py = center_y + dy;
}

static void draw_frame(void)
{
    static const int centers[2] = { LEFT_X, RIGHT_X };
    int i;

    color(GR_COLOR_WHITE);
    GrFillRect(win, gc, 0, 0, EYES_W, EYES_H);

    for (i = 0; i < 2; ++i) {
        oval(centers[i], EYE_Y, EYE_RX, EYE_RY, GR_COLOR_BLACK);
        oval(centers[i], EYE_Y, EYE_RX - 2, EYE_RY - 2, GR_COLOR_WHITE);
    }
    old_px[0] = old_px[1] = -100;
    old_py[0] = old_py[1] = -100;
    drawn = 1;
}

static void update_eyes(void)
{
    static const int centers[2] = { LEFT_X, RIGHT_X };
    GR_SCREEN_INFO screen;
    GR_WINDOW_INFO info;
    int i, px, py;

    if (!drawn)
        return;
    GrGetScreenInfo(&screen);
    GrGetWindowInfo(win, &info);

    for (i = 0; i < 2; ++i) {
        pupil_position(centers[i], EYE_Y,
                       screen.xpos - info.x, screen.ypos - info.y,
                       &px, &py);
        if (px == old_px[i] && py == old_py[i])
            continue;

        /* Old pupils remain safely within the inner white ellipse. */
        if (old_px[i] >= 0)
            oval(old_px[i], old_py[i], PUPIL_RX, PUPIL_RY, GR_COLOR_WHITE);
        oval(px, py, PUPIL_RX, PUPIL_RY, GR_COLOR_BLACK);
        old_px[i] = px;
        old_py[i] = py;
    }
}

int main(void)
{
    GR_EVENT event;

    if (GrOpen() < 0) {
        fprintf(stderr, "nxeyes: cannot connect to Nano-X\\n");
        return 1;
    }

    win = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, "Eyes",
                        GR_ROOT_WINDOW_ID, -1, -1,
                        EYES_W, EYES_H, GR_COLOR_WHITE);
    gc = GrNewGC();
    GrSelectEvents(win, GR_EVENT_MASK_EXPOSURE |
                        GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(win);

    for (;;) {
        GrGetNextEventTimeout(&event, POLL_MS);
        if (event.type == GR_EVENT_TYPE_CLOSE_REQ)
            break;
        if (event.type == GR_EVENT_TYPE_EXPOSURE)
            draw_frame();
        update_eyes();
    }

    GrDestroyGC(gc);
    GrDestroyWindow(win);
    GrClose();
    return 0;
}
