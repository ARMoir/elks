/*
 * ELKS CDE Front Panel - minimal Nano-X shell for 8086/286 and 2 MB RAM.
 *
 * Inspired by the IBM AIX / OSF Common Desktop Environment Front Panel.
 * This is NOT CDE or Motif; it uses the existing 16-bit Nano-X client API.
 * The existing nano-X / NanoWM server and PS/2 mouse are left untouched.
 *
 * Small fixed-size tables, no images, no dynamic allocation, no X11.
 * The drawer enumerates known GUI programs installed on this floppy once
 * at startup. The four CDE-style buttons select BACKDROP colors, not real
 * independent workspaces (that is a separate future feature).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "nano-X.h"
#include "nxcolors.h"

#define PANEL_WIDTH 480
#define PANEL_HEIGHT 62
#define PANEL_Y_OFFSET 65
#define DRAWER_WIDTH 146
#define ROW_HEIGHT 18
#define MAX_APPS 12
#define QUICK_COUNT 4
#define POLL_MS 4500L

struct app_info {
    const char *label;
    const char *path;
};

/* Only binaries actually present on the target filesystem will show. */
static const struct app_info candidates[] = {
    { "Terminal",   "/bin/nxterm"   },
    { "Eyes",       "/bin/nxeyes"   },
    { "Files",      "/bin/nxfiles"  },
    { "Calculator", "/bin/nxcalc"   },
    { "Clock",      "/bin/nxclock"  },
    { "Tetris",     "/bin/nxtetris" },
    { "Mines",      "/bin/nxmine"   },
    { "World",      "/bin/nxworld"  },
    { "Editor",     "/bin/nxedit"   },
    { NULL, NULL }
};

static struct app_info apps[MAX_APPS];
static int app_count;
static int quick[QUICK_COUNT];
static int view = 0;
static int drawer_open;
static GR_WINDOW_ID panel, drawer;
static GR_GC_ID gc;
static int panel_x, panel_y, panel_width;
static int drawer_height;
static int last_minute = -1;
static GR_COLOR backdrop[4] = {
    GR_COLOR_DARKSLATEGRAY,
    GR_COLOR_SLATEGRAY,
    GR_COLOR_STEELBLUE,
    GR_COLOR_MIDNIGHTBLUE
};

static void reapchild(int sig)
{
    (void)sig;
    signal(SIGCHLD, reapchild);
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

static void quit_desktop(void)
{
    /* Match the upstream nxstart behaviour: stop desktop client group. */
    signal(SIGTERM, SIG_IGN);
    kill(-getpid(), SIGTERM);
    GrClose();
    exit(0);
}

static void find_installed_apps(void)
{
    int i, j;

    app_count = 0;
    for (i = 0; i < QUICK_COUNT; ++i)
        quick[i] = -1;

    for (i = 0; candidates[i].label && app_count < MAX_APPS - 1; ++i) {
        if (access(candidates[i].path, F_OK) != 0)
            continue;
        apps[app_count] = candidates[i];
        ++app_count;
    }

    /* Preserve the preferred order of quick launch controls. */
    for (j = 0; j < QUICK_COUNT && j < app_count; ++j)
        quick[j] = j;
}

static void setcolor(GR_COLOR fg)
{
    GrSetGCForeground(gc, fg);
}

static void fill(GR_WINDOW_ID w, int x, int y, int width, int height,
                 GR_COLOR color)
{
    setcolor(color);
    GrFillRect(w, gc, x, y, width, height);
}

static void line(GR_WINDOW_ID w, int x1, int y1, int x2, int y2,
                 GR_COLOR color)
{
    setcolor(color);
    GrLine(w, gc, x1, y1, x2, y2);
}

static void label(GR_WINDOW_ID w, int x, int y, const char *s, GR_COLOR c)
{
    setcolor(c);
    GrText(w, gc, x, y, (void *)s, strlen(s), GR_TFASCII);
}

static void relief(GR_WINDOW_ID w, int x, int y, int width, int height,
                   int pressed)
{
    GR_COLOR top = pressed ? GR_COLOR_DIMGRAY : GR_COLOR_WHITE;
    GR_COLOR bottom = pressed ? GR_COLOR_WHITE : GR_COLOR_DIMGRAY;

    line(w, x, y, x + width - 1, y, top);
    line(w, x, y, x, y + height - 1, top);
    line(w, x + width - 1, y, x + width - 1, y + height - 1, bottom);
    line(w, x, y + height - 1, x + width - 1, y + height - 1, bottom);
}

static void icon(GR_WINDOW_ID w, int x, int y, int type)
{
    /* Simple 16-color low-overhead pixel glyphs, 12 x 12. */
    if (type == 0) {                  /* applications / folder */
        fill(w, x, y + 3, 12, 8, GR_COLOR_NAVY);
        fill(w, x + 1, y + 4, 10, 6, GR_COLOR_LIGHTSTEELBLUE);
        line(w, x + 2, y + 2, x + 6, y + 2, GR_COLOR_WHITE);
    } else if (type == 1) {           /* terminal */
        fill(w, x, y, 12, 11, GR_COLOR_DARKSLATEGRAY);
        line(w, x + 2, y + 3, x + 5, y + 5, GR_COLOR_WHITE);
        line(w, x + 2, y + 7, x + 8, y + 7, GR_COLOR_WHITE);
    } else if (type == 2) {           /* eyes */
        fill(w, x + 1, y + 3, 4, 6, GR_COLOR_WHITE);
        fill(w, x + 7, y + 3, 4, 6, GR_COLOR_WHITE);
        fill(w, x + 3, y + 5, 2, 3, GR_COLOR_BLACK);
        fill(w, x + 9, y + 5, 2, 3, GR_COLOR_BLACK);
    } else if (type == 3) {           /* calculator / generic tool */
        fill(w, x + 2, y, 9, 12, GR_COLOR_DIMGRAY);
        fill(w, x + 3, y + 1, 7, 4, GR_COLOR_LIGHTSTEELBLUE);
        fill(w, x + 4, y + 7, 2, 2, GR_COLOR_WHITE);
        fill(w, x + 8, y + 7, 2, 2, GR_COLOR_WHITE);
    } else {                          /* clock / other app */
        fill(w, x + 1, y + 1, 10, 10, GR_COLOR_WHITE);
        line(w, x + 6, y + 3, x + 6, y + 6, GR_COLOR_BLACK);
        line(w, x + 6, y + 6, x + 9, y + 6, GR_COLOR_BLACK);
    }
}

static void draw_button(int x, int width, const char *caption,
                        int glyph, int active)
{
    fill(panel, x, 18, width, 38,
         active ? GR_COLOR_LIGHTGRAY : GR_COLOR_GRAY);
    relief(panel, x, 18, width, 38, active);
    icon(panel, x + (width - 12)/2, 22, glyph);
    label(panel, x + 5, 50, caption, GR_COLOR_BLACK);
}

static void draw_clock(void)
{
    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    char buf[15];

    if (!tm_now)
        return;
    if (tm_now->tm_min == last_minute)
        return;
    last_minute = tm_now->tm_min;
    sprintf(buf, "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
    fill(panel, panel_width - 70, 2, 66, 13, GR_COLOR_DARKSLATEGRAY);
    label(panel, panel_width - 50, 12, buf, GR_COLOR_WHITE);
}

static void draw_panel(void)
{
    int i, x, idx;

    fill(panel, 0, 0, panel_width, PANEL_HEIGHT, GR_COLOR_GRAY);
    relief(panel, 0, 0, panel_width, PANEL_HEIGHT, 0);
    fill(panel, 3, 2, panel_width - 6, 13, GR_COLOR_DARKSLATEGRAY);
    label(panel, 11, 12, "ELKS   AIX-style Front Panel", GR_COLOR_WHITE);

    draw_button(7, 60, "Apps", 0, drawer_open);
    for (i = 0; i < QUICK_COUNT; ++i) {
        x = 72 + 59 * i;
        idx = quick[i];
        if (idx < 0) {
            fill(panel, x, 18, 55, 38, GR_COLOR_DARKGRAY);
            relief(panel, x, 18, 55, 38, 1);
        } else {
            /* Choice of pixel glyph independent of optional app order. */
            int glyph = !strcmp(apps[idx].label, "Terminal") ? 1 :
                        !strcmp(apps[idx].label, "Eyes") ? 2 :
                        !strcmp(apps[idx].label, "Files") ? 0 :
                        !strcmp(apps[idx].label, "Calculator") ? 3 : 4;
            draw_button(x, 55,
                        !strcmp(apps[idx].label, "Calculator") ? "Calc" :
                        !strcmp(apps[idx].label, "Terminal") ? "Term" :
                        apps[idx].label,
                        glyph, 0);
        }
    }

    /* CDE workspace-switch visual language, implemented as four
       backdrop/palette choices. Does not claim virtual desktop isolation. */
    label(panel, 322, 14, "BACKDROP", GR_COLOR_BLACK);
    for (i = 0; i < 4; ++i) {
        int bx = 313 + (i & 1) * 52;
        int by = 20 + (i >> 1) * 18;
        fill(panel, bx, by, 48, 17,
             i == view ? GR_COLOR_LIGHTSTEELBLUE : GR_COLOR_GRAY);
        relief(panel, bx, by, 48, 17, i == view);
        label(panel, bx + 17, by + 12,
              i == 0 ? "1" : i == 1 ? "2" : i == 2 ? "3" : "4",
              GR_COLOR_BLACK);
    }

    draw_button(423, 50, "Exit", 0, 0);
    last_minute = -1;
    draw_clock();
}

static void apply_backdrop(void)
{
    GR_WM_PROPERTIES props;
    memset(&props, 0, sizeof(props));
    props.flags = GR_WM_FLAGS_BACKGROUND;
    props.background = backdrop[view];
    GrSetWMProperties(GR_ROOT_WINDOW_ID, &props);
    GrClearWindow(GR_ROOT_WINDOW_ID, GR_TRUE);
}

static void drawer_draw(void)
{
    int i;
    fill(drawer, 0, 0, DRAWER_WIDTH, drawer_height, GR_COLOR_GRAY);
    relief(drawer, 0, 0, DRAWER_WIDTH, drawer_height, 0);
    fill(drawer, 3, 3, DRAWER_WIDTH - 6, 17, GR_COLOR_DARKSLATEGRAY);
    label(drawer, 9, 16, "Applications", GR_COLOR_WHITE);
    for (i = 0; i < app_count; ++i) {
        line(drawer, 4, 20 + i*ROW_HEIGHT, DRAWER_WIDTH - 5,
             20 + i*ROW_HEIGHT, GR_COLOR_LIGHTGRAY);
        label(drawer, 10, 34 + i*ROW_HEIGHT,
              apps[i].label, GR_COLOR_BLACK);
    }
    line(drawer, 4, 20 + app_count*ROW_HEIGHT, DRAWER_WIDTH - 5,
         20 + app_count*ROW_HEIGHT, GR_COLOR_DIMGRAY);
    label(drawer, 10, 34 + app_count*ROW_HEIGHT,
          "Exit to shell", GR_COLOR_BLACK);
}

static void show_drawer(int show)
{
    drawer_open = show;
    if (show)
        GrMapWindow(drawer);
    else
        GrUnmapWindow(drawer);
    draw_panel();
}

static void launch_app(const char *path)
{
    char *argv[2];
    argv[0] = (char *)path;
    argv[1] = NULL;
    if (vfork() == 0) {
        execv(path, argv);
        _exit(127);
    }
}

static void on_panel_click(int x, int y)
{
    int i;

    if (y < 18 || y >= 57)
        return;
    if (x >= 7 && x < 67) {
        show_drawer(!drawer_open);
        return;
    }
    if (x >= 72 && x < 308) {
        i = (x - 72) / 59;
        if (i < QUICK_COUNT && quick[i] >= 0) {
            if (drawer_open)
                show_drawer(0);
            launch_app(apps[quick[i]].path);
        }
        return;
    }
    if (x >= 313 && x < 413 && y >= 20 && y < 56) {
        i = (x - 313) / 52 + (y >= 38 ? 2 : 0);
        if (i >= 0 && i < 4) {
            view = i;
            apply_backdrop();
            draw_panel();
        }
        return;
    }
    if (x >= 423 && x < 473)
        quit_desktop();
}

int main(void)
{
    GR_SCREEN_INFO screen;
    GR_EVENT event;
    int row;

    signal(SIGCHLD, reapchild);
    signal(SIGTERM, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    setsid();

    if (GrOpen() < 0) {
        fprintf(stderr, "nxcde: cannot connect to Nano-X\n");
        return 1;
    }

    find_installed_apps();
    GrGetScreenInfo(&screen);
    panel_width = screen.cols < PANEL_WIDTH + 4 ? screen.cols - 4 :
                  PANEL_WIDTH;
    if (panel_width < PANEL_WIDTH) {
        /* This layout is designed for 640x350/480 VGA; fail safely
           rather than offer inaccessible controls on tiny screens. */
        fprintf(stderr, "nxcde: requires a 480-pixel wide desktop\n");
        GrClose();
        return 1;
    }
    panel_x = (screen.cols - panel_width) / 2;
    panel_y = screen.rows - PANEL_Y_OFFSET;
    if (panel_y < 0)
        panel_y = 0;
    drawer_height = 22 + (app_count + 1) * ROW_HEIGHT + 4;
    if (drawer_height > panel_y - 4)
        drawer_height = panel_y - 4;

    panel = GrNewWindowEx(GR_WM_PROPS_NODECORATE |
                          GR_WM_PROPS_NOAUTOMOVE |
                          GR_WM_PROPS_NORESIZE,
                          "ELKS Front Panel", GR_ROOT_WINDOW_ID,
                          panel_x, panel_y, panel_width, PANEL_HEIGHT,
                          GR_COLOR_GRAY);
    drawer = GrNewWindowEx(GR_WM_PROPS_NODECORATE |
                           GR_WM_PROPS_NOAUTOMOVE |
                           GR_WM_PROPS_NORESIZE,
                           "Applications", GR_ROOT_WINDOW_ID,
                           panel_x + 6, panel_y - drawer_height,
                           DRAWER_WIDTH, drawer_height,
                           GR_COLOR_GRAY);

    gc = GrNewGC();
    GrSetGCUseBackground(gc, GR_FALSE);
    GrSetGCFont(gc, GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL));
    GrSelectEvents(panel, GR_EVENT_MASK_EXPOSURE |
                          GR_EVENT_MASK_BUTTON_DOWN |
                          GR_EVENT_MASK_CLOSE_REQ);
    GrSelectEvents(drawer, GR_EVENT_MASK_EXPOSURE |
                           GR_EVENT_MASK_BUTTON_DOWN);
    apply_backdrop();
    GrMapWindow(panel);

    for (;;) {
        memset(&event, 0, sizeof(event));
        GrGetNextEventTimeout(&event, POLL_MS);

        if (event.type == GR_EVENT_TYPE_EXPOSURE) {
            if (event.exposure.wid == panel)
                draw_panel();
            else if (event.exposure.wid == drawer)
                drawer_draw();
        } else if (event.type == GR_EVENT_TYPE_BUTTON_DOWN) {
            if (event.button.wid == panel) {
                on_panel_click(event.button.x, event.button.y);
            } else if (event.button.wid == drawer) {
                row = (event.button.y - 20) / ROW_HEIGHT;
                if (event.button.y >= 20 && row >= 0) {
                    if (row < app_count) {
                        show_drawer(0);
                        launch_app(apps[row].path);
                    } else if (row == app_count) {
                        quit_desktop();
                    }
                }
            } else if (drawer_open) {
                show_drawer(0);
            }
        } else if (event.type == GR_EVENT_TYPE_CLOSE_REQ) {
            quit_desktop();
        } else if (event.type == GR_EVENT_TYPE_TIMEOUT) {
            draw_clock();
        }
    }
}
