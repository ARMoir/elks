/* Exercise the real display adapter and game engine against a fake screen. */
#include <assert.h>
#include "nxinvaders.c"
#define main invaders_game_main
#include "invaders-game.c"
#undef main

static char screen_cells[HEIGHT][WIDTH];
static int draw_calls, draw_chars, pending_expose;

int GrOpen(void) { return 1; }
void GrClose(void) {}
void GrGetScreenInfo(GR_SCREEN_INFO *p) { p->cols = 640; p->rows = 480; }
GR_FONT_ID GrCreateFontEx(const char *n, GR_COORD h, GR_COORD w, GR_LOGFONT *p) { return 1; }
void GrGetFontInfo(GR_FONT_ID f, GR_FONT_INFO *p) { p->maxwidth = 6; p->height = 13; }
GR_WINDOW_ID GrNewWindowEx(GR_WM_PROPS p, const char *t, GR_WINDOW_ID parent,
    GR_COORD x, GR_COORD y, GR_SIZE w, GR_SIZE h, GR_COLOR bg) { return 2; }
GR_GC_ID GrNewGC(void) { return 3; }
void GrSetGCFont(GR_GC_ID g, GR_FONT_ID f) {}
void GrSetGCForeground(GR_GC_ID g, GR_COLOR c) {}
void GrSetGCBackground(GR_GC_ID g, GR_COLOR c) {}
void GrSetGCUseBackground(GR_GC_ID g, GR_BOOL b) { assert(b); }
void GrSelectEvents(GR_WINDOW_ID w, GR_EVENT_MASK m) {}
void GrMapWindow(GR_WINDOW_ID w) {}
void GrFlush(void) {}
void GrRaiseWindow(GR_WINDOW_ID w) {}
void GrSetFocus(GR_WINDOW_ID w) {}
void GrGetWindowInfo(GR_WINDOW_ID w, GR_WINDOW_INFO *p) { p->parent = 1; }
void GrGetNextEventTimeout(GR_EVENT *p, GR_TIMEOUT t)
{
    p->type = pending_expose ? GR_EVENT_TYPE_EXPOSURE : GR_EVENT_TYPE_NONE;
    pending_expose = 0;
}
void GrText(GR_DRAW_ID w, GR_GC_ID g, GR_COORD x, GR_COORD y,
    void *s, GR_COUNT n, GR_TEXTFLAGS flags)
{
    int col = x / 6, row = y / 13;
    assert(col >= 0 && row >= 0 && row < HEIGHT && col + n <= WIDTH);
    memcpy(&screen_cells[row][col], s, n);
    ++draw_calls;
    draw_chars += n;
}

int main(void)
{
    int tick, chars;
    assert(initscr());
    game.screenCols = COLS;
    initGame();
    paintIntro();
    nxinvaders_present();
    assert(draw_calls == HEIGHT && draw_chars == WIDTH * HEIGHT);
    assert(!memcmp(cells, screen_cells, sizeof(cells)));
    draw_calls = draw_chars = 0;
    nxinvaders_present();
    assert(draw_calls == 0); /* idle frame emits no text drawing requests */

    game.state = STATE_PLAY;
    game.lives = 3;
    resetShields();
    resetAliens();
    game.timer = 0;
    erase();
    paintShelters();
    nxinvaders_present();
    draw_chars = 0;
    for (tick = 0; tick < 100; ++tick) {
        handleTimer();
        nxinvaders_present();
        assert(!memcmp(cells, screen_cells, sizeof(cells)));
    }
    chars = draw_chars;
    assert(chars < WIDTH * HEIGHT * 100 / 4);
    draw_calls = draw_chars = 0;
    pending_expose = 1;
    assert(getch() == ERR);
    nxinvaders_present();
    assert(draw_chars == WIDTH * HEIGHT); /* uncovered content fully restored */
    assert(!memcmp(cells, screen_cells, sizeof(cells)));
    draw_calls = 0;
    nxinvaders_present();
    assert(draw_calls == 0); /* no exposure feedback loop */
    printf("PASS: 100 game ticks, %d chars drawn vs 200000 for full redraws; "
           "idle and exposure recovery verified\n", chars);
    return 0;
}
