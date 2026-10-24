# CDE Front Panel — popup and active file-window stacking

This branch is based on the CDE v3 themed selector test and does **not**
change any hardware-verified release.

## Why the Applications drawer went behind other windows

The Apps drawer is a separate undecorated root-level Nano-X window.
Mapping it with GrMapWindow() does not raise it above existing NanoWM
application frames. The native Nano-X sibling stacking API is GrRaiseWindow().

## Changes

- Clicking the Front Panel raises the panel (without stealing focus).
- Opening Apps raises the panel, maps the drawer, then raises the drawer
  above every other root sibling; hiding Apps unmaps the drawer.
- Existing `nxselect -b` Files manager now raises its **parent frame**
  on click, not just its reparented child, and restores keyboard focus.
- Original nxselect chooser behavior and file selection remain unchanged.
- No constant reordering, redraw timers or new memory allocations.
- No change to NanoWM's existing titlebar/window move behavior.

## Verify on real PS/2

1. Run `nxstart`, launch Terminal, XEyes, and Files.
2. Place a large Terminal window over the area where Apps opens.
3. Click Apps in the bottom Front Panel. Its drawer should be fully
   visible **above** Terminal and should accept clicks normally.
4. Close Apps and reopen several times, including after moving Terminal.
5. Move Files so it overlaps Terminal. Click the visible portion of the
   Files window: the entire decorated Files frame should come forward.
6. Click a file, preview, go Back. Test window close and return to shell.
7. Move XEyes and confirm its pupils follow the mouse after dragging.

Rollback: ibm-ps2-cde-front-panel-working-20261023.
