/*
 * CDE-inspired window chrome on the ordinary 80x25 hardware text console.
 * Nano-X is stopped before this starts. No graphics server, PTY or redraw
 * timer: draw the frame once, then constrain game output to its interior.
 */
#include "curses.h"
#include "unikey.h"
#define GAMEFRAME_IMPLEMENTATION
#include "gameframe.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define FRAME_COLS 76
#define FRAME_LINES 21
#define FRAME_X 2
#define FRAME_Y 2

static int framed, saved_cols, saved_lines, game_x, game_y;
static const char *title, *controls;
static const char blanks[] =
    "                                                                            ";

static void position(int y, int x)
{
    printf("\033[%d;%dH", y + 1, x + 1);
}

static void draw_frame(void)
{
    int y, x;
    /* The console uses its CP437 font; no downloaded glyphs or bitmap work. */
    printf("\033[0;37;44m\033[2J\033[H ELKS Desktop\033[K");
    position(1, 1);
    printf("\033[97;46m\332%s\277", blanks);
    position(1, 3);
    printf("%s", title);
    position(1, 60);
    printf("[ Ctrl+X Desktop ]");
    for (y = FRAME_Y; y < FRAME_Y + FRAME_LINES; ++y) {
        position(y, 1);
        printf("\033[30;47m\263");
        position(y, 78);
        putchar('\263');
    }
    position(23, 1);
    putchar('\300');
    for (x = 0; x < FRAME_COLS; ++x) putchar('\304');
    putchar('\331');
    position(23, 3);
    printf(" %s ", controls);
    position(24, 0);
    printf("\033[97;46m ELKS  |  %s  |  Ctrl+X: close game and return to Desktop\033[K", title);
    printf("\033[0;37;40m\033[?25l");
}

void *gameframe_initscr(void)
{
    void *win = initscr();
    const char *game = getenv("NXCDE_GAME");
    framed = 0;
    if (!win || !iselksconsole || !game || COLS < 80 || LINES < 25)
        return win;
    if (!strcmp(game, "invaders")) {
        title = "Space Invaders";
        controls = "j/k Move   Space Start / Fire   Q Close";
    } else if (!strcmp(game, "pong")) {
        title = "Pong";
        controls = "Up/Down or k/j Move   P Pause   Q Close";
    } else return win;
    saved_cols = COLS;
    saved_lines = LINES;
    COLS = FRAME_COLS;
    LINES = FRAME_LINES;
    framed = 1;
    draw_frame();
    gameframe_erase();
    return win;
}

void gameframe_move(int y, int x)
{
    if (!framed) {
        move(y, x);
        return;
    }
    game_y = y;
    game_x = x;
    if (y >= 0 && y < LINES && x < COLS)
        position(y + FRAME_Y, (x < 0 ? 0 : x) + FRAME_X);
}

/* Clip even sprites that straddle the edge, so they cannot erase the frame. */
static void text(const char *buf, int n)
{
    int skip = game_x < 0 ? -game_x : 0;
    int count = n - skip;
    if (game_x + skip + count > COLS) count = COLS - game_x - skip;
    if (game_y >= 0 && game_y < LINES && count > 0)
        fwrite(buf + skip, 1, count, stdout);
    game_x += n;
}

int gameframe_addch(int ch)
{
    if (!framed) return addch(ch);
    if (ch == '\n') gameframe_move(game_y + 1, 0);
    else {
        if (game_y >= 0 && game_y < LINES && game_x >= 0 && game_x < COLS)
            putchar(ch);
        ++game_x;
    }
    return OK;
}

int gameframe_mvaddch(int y, int x, int ch)
{
    gameframe_move(y, x);
    return gameframe_addch(ch);
}

static void format(const char *fmt, va_list args)
{
    char buf[128];
    if (!framed) vfprintf(stdout, fmt, args);
    else {
        vsnprintf(buf, sizeof(buf), fmt, args);
        text(buf, strlen(buf));
    }
}

void gameframe_printw(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    format(fmt, args);
    va_end(args);
}

void gameframe_mvprintw(int y, int x, const char *fmt, ...)
{
    va_list args;
    gameframe_move(y, x);
    va_start(args, fmt);
    format(fmt, args);
    va_end(args);
}

void gameframe_clrtoeol(void)
{
    int x = game_x < 0 ? 0 : game_x;
    if (!framed) clrtoeol();
    else if (game_y >= 0 && game_y < LINES && x < COLS) {
        position(game_y + FRAME_Y, x + FRAME_X);
        fwrite(blanks, 1, COLS - x, stdout);
        gameframe_move(game_y, game_x);
    }
}

void gameframe_erase(void)
{
    int y;
    if (!framed) {
        erase();
        return;
    }
    printf("\033[0;37;40m");
    for (y = 0; y < LINES; ++y) {
        position(y + FRAME_Y, FRAME_X);
        fwrite(blanks, 1, COLS, stdout);
    }
    gameframe_move(0, 0);
}

void gameframe_endwin(void)
{
    if (framed) {
        position(24, 0);
        printf("\033[97;46m Opening ELKS Desktop...\033[K");
        /* tty_restore emits a newline, so leave its cursor off the last row. */
        position(0, 0);
        COLS = saved_cols;
        LINES = saved_lines;
        framed = 0;
    }
    endwin();
}

int gameframe_getch(void)
{
    int key = getch();
    /* Handle this here so Invaders' input drain and Pong's pause cannot eat it. */
    if (framed && key == 24) {       /* Ctrl+X */
        gameframe_endwin();
        exit(0);
    }
    return key;
}
