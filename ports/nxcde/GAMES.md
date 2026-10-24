# Games and the console handoff

Real PS/2 feedback found both windowed Pong and windowed Invaders too slow,
including the native Invaders adapter. Invaders ran acceptably without the UI.
Their menu entries now use the original console programs with Nano-X stopped.

| Apps entry | Program | Interface | Controls |
| --- | --- | --- | --- |
| Mines | `/bin/nxmine` | Nano-X window | Mouse and on-screen controls |
| Invaders (TTY) | `/bin/invaders` | Full-screen console | j/k move; Space starts/fires; q quits |
| Pong (TTY) | `/bin/ttypong` | Full-screen console | Up/down or k/j; p pauses; q quits |

## Starting a console game

1. Close other application windows, including terminals and Files. This
   requirement avoids terminating an editor or another app with work open.
2. Select the game from Apps, then choose Play (or press Enter).
3. The panel closes its Nano-X connection, stops its process group and waits
   for the server to finish restoring VGA, keyboard and mouse state.
4. The `nxcde` session wrapper runs the original game directly on the console.
5. Quitting the game restores terminal settings and starts a fresh desktop.

F5 opens the Invaders launch prompt and F6 opens Pong when the Front Panel has
keyboard focus. Escape cancels the prompt. If another application is open,
the prompt explains that it must be closed; it never forcibly closes it.

Root console login now starts `/bin/nxcde`. Use `nxcde` for manual startup as
well. Exit still returns to the login shell and does not reopen the desktop.
The `/root/nox` opt-out and serial/PTY login exclusions are unchanged.

The custom Files browser, repaired nxselect, other text-application shortcuts,
and graphical Mines remain. `/bin/nxinvaders` is retained as an experimental
manual command, but is no longer the Invaders menu entry.

## Verification

Check both full-screen game launches and the return to the desktop, including
keyboard input, mouse input after return, and repeat launches. Verify an open
application blocks the handoff and Escape cancels without closing anything.

Emulation can check the transition and terminal restoration; gameplay speed
on the original 2 MB PS/2 still requires a hardware test.
