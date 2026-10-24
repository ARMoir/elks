# Three small desktop games

This image keeps the console-login desktop startup, text applications,
and popup/window stacking fixes from the preceding experimental image.

The Apps drawer offers:

| Entry | Program | Interface | Controls |
| --- | --- | --- | --- |
| Mines | `/bin/nxmine` | Native Nano-X window | Mouse; use the on-screen controls |
| Space Invaders | `/bin/nxinvaders` | Native Nano-X window | Left/right or j/k move; Space starts/fires; q or Esc quits |
| Pong | `/bin/ttypong` | Titled nxterm window | Up/down arrows or k/j; p pauses; q quits |

Mines is already built by upstream Microwindows. The PS/2 image packages
it using a diagnostic-image-only tag. Pong uses nxterm's direct `-e` interface.
Invaders now uses the existing game's logic with a native Nano-X display:
only changed character runs are drawn, once per game tick. It needs no PTY,
ANSI interpreter or extra shell. The original `/bin/invaders` stays available
for use at the plain console. No extra game data files are needed.

Entries are still filtered against installed binaries. Quick-launch ordering
is unchanged, and each game can be closed using its window close button.

## Verify on the PS/2

1. Log in and check the desktop opens, then open Apps and launch each game.
2. Check Mines accepts mouse clicks and its controls remain accessible.
3. Check Invaders movement/firing with j/k and Space, and Pong movement,
   pause and quit. For Pong, also test its arrow-key alternatives.
4. Overlap and move windows; check redraw and click-to-focus behavior.
5. Quit and relaunch several times to check terminal/PTY cleanup.
6. Exit the desktop and check for orphan game processes using `ps`.

Start with one game at a time on 2 MB hardware. Build and image checks cannot
establish animation speed, terminal rendering, or real-hardware stability.

## Files and performance follow-up

The file selector now creates its client with GrNewWindowEx and zero client
border, so NanoWM can add its own frame without first unmapping/remapping a
legacy bordered window. Its fixed layout is no longer resizable. Directory
loading happens before mapping, and the fixed font is set explicitly.

The native Invaders adapter keeps 4 KB of character grids and has no full
pixel framebuffer. A host test runs the actual game engine for 100 ticks,
checks rendered contents against the logical screen, verifies unchanged
frames produce no text requests, and checks a full redraw after exposure.
This measures rendering work, not frames per second on the real PS/2.
