# Games and hardware-text windows

Real PS/2 feedback found windowed Pong and Invaders too slow, including the
native Nano-X Invaders adapter. Invaders ran acceptably without the UI.
The game shortcuts now stop Nano-X and open the original console games in
CDE-inspired hardware-text frames, then reload the desktop when they close.

| Apps entry | Program | Interface | Controls |
| --- | --- | --- | --- |
| Mines | `/bin/nxmine` | Nano-X window | Mouse and on-screen controls |
| Invaders | `/bin/invaders` | Hardware-text window | j/k move; Space starts/fires; Q closes |
| Pong | `/bin/ttypong` | Hardware-text window | Up/down or k/j move; P pauses; Q closes |

**Ctrl+X closes either text game and returns to the desktop.** The shortcut
is displayed in the title bar and desktop strip. It also works while Pong
is paused or Invaders is draining queued input. This is a close/reopen
transition, not a suspended game or a live graphical desktop underneath.
The text title bar and strip do not accept mouse clicks.

## Opening and closing a game

1. Close other application windows, including terminals and Files. This
   requirement avoids terminating an editor or another app with work open.
2. Select Invaders or Pong in Apps. With no other windows open, it launches
   directly. I and P are equivalent shortcuts when the Front Panel has focus.
3. The panel closes its Nano-X connection, stops its process group and waits
   for the server to finish restoring VGA, keyboard and mouse state.
4. The `nxcde` session runs the game on the original console with a temporary
   `NXCDE_GAME` setting. There is no graphics server or terminal emulator
   running during play. A static text-mode title bar, borders and desktop
   strip surround a clipped 76-by-21-character playfield on the 80-by-25 screen.
5. Ctrl+X or Q closes the game, displays "Opening ELKS Desktop..." and starts
   a fresh desktop. Loading time depends on the real floppy drive.

If another application is open, the launch prompt explains that it must be
closed; it never forcibly closes it. Escape dismisses that prompt.
Root console login starts `/bin/nxcde`; use `nxcde` for manual startup too.
Exit still returns to the login shell. The `/root/nox` opt-out and serial/PTY
login exclusions are unchanged.

Running `invaders` or `ttypong` from an ordinary shell retains the unframed
console interface. Framing also stays disabled on terminal emulators and
consoles smaller than 80-by-25. Only these two games link the frame adapter;
other programs keep their existing curses code. Pong now paints only changed
cells and accepts Q while paused. Invaders' intro fits the smaller playfield.

The custom Files browser, repaired nxselect, other text-application shortcuts,
and graphical Mines remain. `/bin/nxinvaders` remains an experimental manual
command, rather than the Apps menu entry.

## Verification

Host checks cover clipping on all edges, clearing without erasing the frame,
no screen scrolling, Ctrl+X and the ordinary-console fallback. The real Pong
engine/renderer runs for 100 ticks and its output is checked against game state;
an unchanged board must produce no character updates. The experimental native
Invaders renderer still passes its existing 100-tick and exposure tests.

Check both launches, title bars, gameplay, Ctrl+X returns (including paused
Pong), repeated launches and the ordinary Q exit on the 2 MB machine. An open
application must block the handoff and Escape must leave it open. Verify the
mouse still works after returning. Emulation can verify transitions and screen
layout; actual PS/2 gameplay speed and floppy loading time require hardware.
