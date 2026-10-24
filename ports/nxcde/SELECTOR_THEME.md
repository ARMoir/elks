# CDE Front Panel v3 — upstream Nano-X selector and system-wide color theme

This experimental build starts from the exact real-hardware-verified CDE
Front Panel branch. The archival release is unchanged.

## Changes

- Reuse Anton Andreev's existing ELKS Nano-X file selector (`nxselect`).
- Add standalone `nxselect -b`/`nxselect --browse` mode invoked as **Files**
  by the CDE Front Panel; folder browsing and page scrolling are inherited.
- Selected files can be viewed as read-only text within the same window.
- Without `-b`, existing nxselect selection protocol is preserved:
  an OK click writes the full selected path to stdout and exits,
  or writes [] when canceled. No edits, renames, deletion or writes.
- CDE palette adjusts the actual NanoWM `SCHEME_NUK16` color table,
  which controls decorated titlebars and frames for all Nano-X clients.
  Uses muted blue/teal titles, gray inactive captions and light borders.
- CDE launcher also preserves XEyes and installed-only app filtering.

## Test on the 2 MB PS/2

1. Boot ELKS from the experimental FAT12 1.44 MB floppy.
2. Run `nxstart`; check panel, mouse and clock.
3. Launch Terminal, Eyes, Calc: check titlebars match the CDE palette
   and all existing window moves and close buttons still work.
4. Open **Files** (launches `nxselect -b`): open /bin, return to /,
   browse /etc, click a text file and choose View. Back returns to list.
5. Move XEyes and verify gaze tracking still follows the cursor.
6. Exit desktop, relaunch, verify socket cleanup still works.

Limitations: browser displays first 70 directory entries, text preview is
paged and read-only, and graphical window decoration colors are static.
Workspace buttons remain backdrop-color selectors, not virtual workspaces.

Rollback: `ibm-ps2-cde-front-panel-working-20261023` (verified source/image).
