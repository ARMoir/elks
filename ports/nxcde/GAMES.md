# Three small desktop games

This image keeps the console-login desktop startup, text applications,
and popup/window stacking fixes from the preceding experimental image.

The Apps drawer offers:

| Entry | Program | Interface | Controls |
| --- | --- | --- | --- |
| Mines | `/bin/nxmine` | Native Nano-X window | Mouse; use the on-screen controls |
| Space Invaders | `/bin/invaders` | Titled nxterm window | Left/right arrows or j/k; Space fires; q quits |
| Pong | `/bin/ttypong` | Titled nxterm window | Up/down arrows or k/j; p pauses; q quits |

Mines is already built by upstream Microwindows. The PS/2 image now packages
it using a diagnostic-image-only tag. Invaders and Pong were already included
on the 1.44 MB image; their new menu entries launch them via nxterm's direct
`-e` program interface. No extra game data files are needed.

Entries are still filtered against installed binaries. Quick-launch ordering
is unchanged, and each game can be closed using its window close button.

## Verify on the PS/2

1. Log in and check the desktop opens, then open Apps and launch each game.
2. Check Mines accepts mouse clicks and its controls remain accessible.
3. Check Invaders movement/firing and Pong movement/pause/quit. If arrow
   keys behave differently, test the letter-key alternatives listed above.
4. Overlap and move windows; check redraw and click-to-focus behavior.
5. Quit and relaunch several times to check terminal/PTY cleanup.
6. Exit the desktop and check for orphan game processes using `ps`.

Start with one game at a time on 2 MB hardware. Build and image checks cannot
establish animation speed, terminal rendering, or real-hardware stability.
