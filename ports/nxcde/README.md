# ELKS Nano-X CDE-inspired Front Panel prototype

An independent tiny CDE-look shell for a 2 MB IBM PS/2.

This is not historical OSF CDE, AIX or Motif. Only the Nano-X launcher
(/bin/nxstart) changes; 830-byte kernel stack, BIOS floppy root, native
PS/2 mouse, Nano-X server, elkfetch and /etc/rc.sys socket cleanup stay intact.

## First experimental layout

- Centered 480 x 62 front panel at the bottom of a 640-wide VGA desktop.
- 16-color beveled/Motif-like palette, no external graphic assets.
- Slide-up Applications drawer with only actually installed GUI programs.
- Four adaptive quick launch buttons for installed GUI applications.
- Live local time in the panel.
- Four functional BACKDROP buttons cycle workspace-style background colors.
  They do NOT implement independent virtual workspaces (future feature).
- Exit returns to the ELKS command shell.
- XEyes retains the moved-window position tracking fix.

## Build

After ports/nxeyes/build.sh run ports/nxcde/build.sh; make -C image
packs the updated nxstart into a separate FAT12 1.44 MB floppy image.

Stable rollback: release ibm-ps2-nxeyes-dynamic-menu-working-20261023.

## Real hardware checks

1. Boot, check elkfetch and absence of KSTACK overflow.
2. Run nxstart; panel should appear bottom-center.
3. Apps shows installed applications; launch XEyes and Terminal.
4. Drag XEyes window and check pupils follow the mouse.
5. Backdrop buttons change color without hiding windows.
6. Exit returns to shell. Restart and confirm Nano-X still starts.

No full-screen wallpaper process, image assets, X11 or Motif dependencies.
