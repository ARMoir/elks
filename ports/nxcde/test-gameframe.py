#!/usr/bin/env python3
"""Check console window bounds and the always-available desktop shortcut."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
tui = root / "elkscmd/tui"
probe = r'''
#include "curses.h"
#define GAMEFRAME_IMPLEMENTATION
#include "gameframe.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int COLS = 80, LINES = 25, iselksconsole = 1;
void *stdscr;
void *initscr(void) { return stdout; }
void endwin(void) { fflush(stdout); }
void move(int y, int x) { printf("\033[%d;%dH", y+1, x+1); }
void erase(void) { printf("\033[H\033[2J"); }
int addch(int ch) { return putchar(ch); }
void clrtoeol(void) { printf("\033[K"); }
int getch(void) { return 24; }
int main(int argc, char **argv) {
    if (!strcmp(argv[1], "terminal")) iselksconsole = 0;
    assert(gameframe_initscr());
    if (!strcmp(argv[1], "normal") || !strcmp(argv[1], "terminal")) {
        assert(COLS == 80 && LINES == 25);
        assert(gameframe_getch() == 24); /* unframed input is untouched */
        gameframe_mvprintw(2, 3, "normal");
        return 0;
    }
    assert(COLS == 76 && LINES == 21);
    if (!strcmp(argv[1], "quit")) {
        gameframe_getch();
        return 17; /* Ctrl+X must exit, even inside an input-drain/pause loop */
    }
    putchar(30);
    gameframe_mvprintw(0, -2, "ABCDE");
    gameframe_mvprintw(20, 74, "WXYZ");
    gameframe_mvaddch(-1, 0, '!');
    gameframe_mvaddch(21, 0, '!');
    gameframe_move(3, 73);
    gameframe_printw("ABCDEF");
    putchar(30);
    gameframe_move(3, 73);
    gameframe_clrtoeol();
    putchar(30);
    gameframe_erase();
    putchar(30);
    gameframe_endwin();
    assert(COLS == 80 && LINES == 25);
    return 0;
}
'''


class Screen:
    def __init__(self):
        self.cells = [[" "] * 80 for _ in range(25)]
        self.x = self.y = 0
        self.scrolls = 0

    def feed(self, data):
        parts = re.split(rb"(\x1b\[[0-9;?]*[A-Za-z])", data)
        for part in parts:
            if part.startswith(b"\x1b["):
                op = chr(part[-1])
                args = part[2:-1].decode()
                if op == "H":
                    coords = [int(x or 1) for x in args.split(";")]
                    self.y = (coords[0] if coords else 1) - 1
                    self.x = (coords[1] if len(coords) > 1 else 1) - 1
                elif op == "J" and args == "2":
                    self.cells = [[" "] * 80 for _ in range(25)]
                elif op == "K":
                    self.cells[self.y][self.x:] = [" "] * (80 - self.x)
                continue
            for c in part.decode("cp437"):
                self.cells[self.y][self.x] = c
                self.x += 1
                if self.x == 80:
                    self.x = 0
                    self.y += 1
                    if self.y == 25:
                        self.scrolls += 1
                        self.cells.pop(0)
                        self.cells.append([" "] * 80)
                        self.y = 24

    def chrome(self):
        return [self.cells[y][x] for y in range(25) for x in range(80)
                if not (2 <= y <= 22 and 2 <= x <= 77)]


with tempfile.TemporaryDirectory() as temp:
    temp = Path(temp)
    (temp / "probe.c").write_text(probe)
    subprocess.run(["gcc", "-std=gnu99", "-Wall", "-Werror", "-I", str(tui),
                    str(temp / "probe.c"), str(tui / "gameframe.c"),
                    "-o", str(temp / "probe")], check=True)
    for game in ("invaders", "pong"):
        env = {**os.environ, "NXCDE_GAME": game}
        data = subprocess.check_output([temp / "probe", "frame"], env=env)
        stages = data.split(bytes([30]))
        assert len(stages) == 5
        screen = Screen()
        screen.feed(stages[0])
        chrome = screen.chrome()
        assert "Ctrl+X Desktop" in "".join(screen.cells[1])
        screen.feed(stages[1])
        assert screen.cells[2][2:5] == list("CDE")
        assert screen.cells[22][76:78] == list("WX")
        assert screen.cells[5][75:78] == list("ABC")
        assert screen.chrome() == chrome
        screen.feed(stages[2])
        assert screen.cells[5][75:78] == list("   ")
        assert screen.chrome() == chrome
        screen.feed(stages[3])
        assert all(c == " " for row in screen.cells[2:23] for c in row[2:78])
        assert screen.chrome() == chrome
        screen.feed(stages[4])
        assert "Opening ELKS Desktop" in "".join(screen.cells[24])
        assert screen.scrolls == 0
        data = subprocess.check_output([temp / "probe", "quit"], env=env)
        assert b"Opening ELKS Desktop" in data
        data = subprocess.check_output([temp / "probe", "terminal"], env=env)
        assert data == b"\033[3;4Hnormal"
    env = dict(os.environ)
    env.pop("NXCDE_GAME", None)
    data = subprocess.check_output([temp / "probe", "normal"], env=env)
    assert data == b"\033[3;4Hnormal"
print("PASS: both game frames, edge clipping, clears, no scrolling, Ctrl+X and plain-console fallback")
