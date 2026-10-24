# ELKS CDE desktop v2 — read-only graphical Files

Based on the exact CRT-verified IBM CDE-style Front Panel source.
The 830-byte stack, BIOS floppy, PS/2 mouse, XEyes fix, elkfetch login display
and automatic rm -f /tmp/nxsock at boot are unchanged.

## Browser behavior

- A Files shortcut is added to the Front Panel and Apps drawer if /bin/nxfiles exists.
- New lightweight read-only Nano-X file browser (no Motif or X11 dependency).
- Browse the root FAT filesystem and subdirectories by clicking rows.
- Up navigates one directory back. Prev/Next move through 11-entry pages.
- Clicking a file opens a read-only preview; Prev/Next move through text pages.
- Back returns from preview; Close dismisses the application.
- No edit, delete, rename, or write actions. Maximum 72 entries per folder.

## Build and test

Build: after ./buildext.sh microwindows, run ports/nxeyes/build.sh,
ports/nxcde/build.sh, then ports/nxfiles/build.sh, and make -C image.

Hardware test on the same PS/2:

1. Boot new FAT12 image, log in, run nxstart.
2. Ensure Files appears in the front panel and drawer.
3. Open Files and navigate /bin, /etc, and back to /.
4. Open an ASCII text file; verify Back/Prev/Next.
5. Confirm no KSTACK alerts, no strange memory pressure, other apps still run.

Stable rollback: ibm-ps2-cde-front-panel-working-20261023.


## Restored as the desktop Files app

The Front Panel and Apps drawer use this custom browser again. It keeps its
original folder/document icons, wider list, Up/Prev/Next/Close buttons and
single-click read-only preview. The window is created without a client
border, uses an explicit fixed font, and raises its NanoWM parent frame and
sets keyboard focus when clicked. The fixed layout cannot be resized.

The current image also retains the repaired nxselect file chooser, native
Space Invaders and automatic desktop startup after root console login.
This browser is restored from the v2 prototype at fb34c2f9bb1682906f75f7dfec0696f70639e942.
