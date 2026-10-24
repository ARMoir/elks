/* Optional hardware-text game window, selected by the nxcde session. */
#ifndef GAMEFRAME_H
#define GAMEFRAME_H

void *gameframe_initscr(void);
void gameframe_endwin(void);
void gameframe_erase(void);
void gameframe_move(int y, int x);
int gameframe_addch(int ch);
int gameframe_mvaddch(int y, int x, int ch);
void gameframe_printw(char *fmt, ...);
void gameframe_mvprintw(int y, int x, const char *fmt, ...);
void gameframe_clrtoeol(void);
int gameframe_getch(void);

/* Only the two games include this header; the shared curses stays unchanged. */
#ifndef GAMEFRAME_IMPLEMENTATION
#define initscr     gameframe_initscr
#define endwin      gameframe_endwin
#define erase       gameframe_erase
#define move        gameframe_move
#define addch       gameframe_addch
#define mvaddch     gameframe_mvaddch
#define printw      gameframe_printw
#define mvprintw    gameframe_mvprintw
#define clrtoeol    gameframe_clrtoeol
#define getch       gameframe_getch
#endif

#endif
