/* Check the real Pong renderer against its game state over moving frames. */
#include <assert.h>
#include <string.h>
#define main pong_program_main
#include "ttypong.c"
#undef main

int COLS = 80, LINES = 25;
void *stdscr;
static int image[25][80], cx, cy, color, writes;
static int input;

void *initscr(void) { return (void *)1; }
void endwin(void) {}
void nodelay(void *w, int flag) {}
void curs_set(int visible) {}
void start_color(void) {}
int use_default_colors(void) { return OK; }
void init_pair(int n, int fg, int bg) {}
void attron(int n) { color = n; }
void attroff(int n) {}
void refresh(void) {}
void erase(void) { memset(image, -1, sizeof(image)); }
void move(int y, int x) { cy = y; cx = x; }
int addch(int ch)
{
    assert(cx >= 0 && cx < COLS && cy >= 0 && cy < LINES);
    assert(ch == (color == BALL ? 'O' : ' '));
    image[cy][cx++] = color;
    ++writes;
    return OK;
}
int mvaddch(int y, int x, int ch) { move(y, x); return addch(ch); }
int getch(void) { int ch = input; input = 'q'; return ch; }

static void check_image(void)
{
    int x, y;
    for (y = 0; y <= FH; ++y)
        for (x = 0; x <= FW; ++x)
            assert(image[y][x + GX] == pong->frame[y][x]);
}

int main(void)
{
    int tick, initial;
    pong = calloc(1, sizeof(*pong));
    assert(pong);
    pong->running = 1;
    pong->ball.a = 1;
    pong->ball.b = -1;
    pong->ball.x = pong->ball.y = 10;
    pong->racket.y = 1;
    init_curses();
    init_frame();
    draw_frame();
    check_image();
    initial = writes;
    draw_frame();
    assert(writes == initial); /* unchanged board emits no characters */
    writes = 0;
    for (tick = 0; tick < 100; ++tick) {
        racket_move(1 + tick % (FH - RACKETL));
        ia_racket();
        manage_ball();
        draw_frame();
        check_image();
    }
    assert(writes < initial * 100 / 10);
    input = 'p';
    key_event();
    assert(!pong->running); /* Q closes even while paused */
    printf("PASS: 100 Pong ticks, %d cells updated vs %d full redraws; idle and paused close verified\n",
           writes, initial * 100);
    free(pong);
    return 0;
}
