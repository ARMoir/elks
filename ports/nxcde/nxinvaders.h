/* Small display adapter for the existing GPL ascii-invaders game logic. */
#ifndef NXCDE_INVADERS_H
#define NXCDE_INVADERS_H

#define ERR (-1)
#define TRUE 1
#define KEY_LEFT  0x101
#define KEY_RIGHT 0x102
#define clear erase
#define cbreak()
#define noecho()
#define keypad(w, flag)
#define curs_set(visible)

extern int LINES, COLS;
void *initscr(void);
void endwin(void);
void refresh(void);
void move(int y, int x);
void erase(void);
void clrtoeol(void);
int addch(int ch);
int mvaddch(int y, int x, int ch);
void printw(const char *fmt, ...);
void mvprintw(int y, int x, const char *fmt, ...);
int getch(void);
void nxinvaders_present(void);
#endif
