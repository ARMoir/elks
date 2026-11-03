# ELKS for Tandy 1000 EX, V20 and the 3-in-1 V2 card

This is a separate, experimental CompactFlash build. It does not replace the
IBM PS/2 floppy image. The target is the Tandy 1000 EX with its NEC V20 CPU,
TGA display and the rkrenicki Tandy EX/HX 3-in-1 expansion card (single serial port).

## Disk and startup

- Raw whole-disk image: 15,998,976 bytes, or 31,248 sectors of 512 bytes.
- This fits the target "16 MB" CF card reporting 31,360 sectors. The earlier
  16 MiB image was too large for that card; use the `smallcard` image instead.
- Active primary FAT16 partition, starting at LBA 63, with 31,184 sectors.
- The partition length is aligned to ELKS's 1 KiB block I/O, including its final block.
- The image ends at a reference cylinder boundary for old CHS BIOS compatibility.
  One padding sector follows the partition within the image; the target card's
  remaining 112 sectors are also unallocated.
- Boot uses the installed XTIDE BIOS, not direct ATA controller accesses.
- The MBR queries BIOS disk geometry and supplies it to the loaded ELKS FAT
  boot sector. It also uses that geometry to find the partition by LBA. This
  permits moving the image to a larger CF card without using the image's
  original CHS values for disk reads. It does not change the XTIDE ROM.
- Root filesystem: `/dev/hda1`. Boot selects 80-column hardware text mode.
- Log in as `root`, with no password. elkfetch runs, then a shell is available.
- Code targets the 8086 instruction set, so the V20 needs no 286 instructions.
- Console startup, one console login, reduced cache/heap settings, no HMA/XMS,
  no network-card probing, no direct floppy DMA driver and no AT RTC utility.

The linked V2 reference documents 640 KB of base system RAM and 96 KB of upper
memory, CF I/O at 300 and BIOS ROM at C0000-C7FFF. This image targets the
single-serial-port variant, using COM1 (3F8/IRQ4) for the mouse. It does not
require a second serial port. The extra RAM at C8000-DFFFF is optional: first
verify that your board revision has RAM at that range. After a successful first
boot, uncomment `umb=0xC800:0x1800` in `/bootopts` to make that 96 KiB available
to ELKS. Do not include the C0000-C7FFF ROM range.

Card documentation:
https://github.com/rkrenicki/Tandy-EX-HX-3in1-V2

## Writing the card

Extract the ZIP, then write the `.img` as a raw image to the entire CF device,
not as a file copied into a DOS partition. Use a card with at least 31,248
512-byte sectors (15,998,976 bytes) of addressable space. This replaces the card's partition
table and existing data. Extra space on a larger card remains unallocated.

On Windows, select `elks-tandy1000ex-cf16-smallcard.img` and the CF card in
Win32 Disk Imager, then Write. It requires 31,248 sectors, so the target card's
31,360 sectors are sufficient. Cancel any insufficient-space warning; do not
truncate the image or continue with the earlier oversized image.
On Linux, after confirming the device name:

```sh
sudo dd if=elks-tandy1000ex-cf16-smallcard.img of=/dev/sdX bs=1M conv=fsync status=progress
```

Replace `/dev/sdX` with the whole CF device, not a numbered partition.
Safely eject the card, insert it into the Tandy and boot through XTIDE.
The FAT16 partition also allows `/bootopts` to be edited from another computer.
Exit applications and run `shutdown` before removing power or the card.

## Applications

From the shell:

```text
fm          text file manager
kilo        text editor (Ctrl+S saves, Ctrl+Q quits)
basic       BASIC interpreter
invaders    console Space Invaders (j/k move, Space fires, Q quits)
ttypong     console Pong (j/k or arrows move, P pauses, Q quits)
elkfetch    system summary
meminfo     memory details
```

The image also includes the selected ELKS file, disk and system utilities,
vi, screen, cron and other small applications. Programs are stored on CF,
not all resident in RAM at once. Networking is disabled in this first build.

## TGA and the optional serial mouse

This build uses TGA's CGA-compatible mode: 640x200, two colors. It does not
implement Tandy's native 16-color graphics, and does not start the VGA/CDE
desktop from the PS/2 image. Graphics stay opt-in for the first hardware test.
The launcher selects the driver's two-color palette so window titles and
controls retain contrast on the monochrome display.
The CGA build also replaces VGA-specific window-copy operations with CGA
pixel operations and preserves the memory-surface format expected by ELKS.
The apps use contrasting colors and layouts sized for the 200-line display.
Nano-X, Tetris, Mines, Clock and Eyes are included with a small launcher sized
for the 200-line display. At the shell, use:

```text
nxcga mouse  serial mouse on the single COM1 port (/dev/ttyS0, 3F8, IRQ4)
nxcga none   keyboard-only launcher
```

In the launcher, keys 1-4 start its four apps. Ctrl+A returns to the console
from any app using Nano-X's built-in exit shortcut. Q or Escape also returns
when the launcher has focus. With a mouse, click its Return to console entry
or close its window. Close app windows first to retain their game progress.
The session manager owns the server and launcher, waits for them and their
apps to exit, restores terminal settings, then clears and homes the text
console before the shell resumes. Ctrl+A and normal window exits share this
cleanup, including when a client loses its server connection.
Tetris uses N for a new game, J/K to move, D/F to rotate, Space to drop,
P/C to pause/continue and Q to close. Mines requires the mouse to play.
No serial login runs, so the mouse has exclusive use of the port. The
`com1`/`com2` arguments remain available only as address overrides if your
card maps its single connector differently; a second connector is not needed.
The driver defaults to the Microsoft serial protocol (`MOUSE_TYPE=ms`). For
a genuine Mouse Systems protocol device, use `MOUSE_TYPE=pc nxcga mouse`.
A passive USB/PS2-to-serial plug only works with a mouse that supports serial.

The BIOS date/time may need to be set manually on a machine without an RTC.
The normal first boot deliberately performs no clock-chip reads via `clock`.

## Building and validation

Run `bash ports/tandy1000/build.sh` from a fresh checkout with the ELKS build
prerequisites installed. The dedicated `tandy-cf16.yml` workflow builds only
this image, uses the cached ia16 compiler, pins Nano-X to a known revision,
and packages the disk, SHA-256 checksum and this document.

The image builder checks its size, MBR signatures and partition bounds, FAT16
cluster count, identical FAT copies and BPB offsets. The build extracts the
actual image's applications, boot options and login profile to verify them.
Emulation can check boot, different BIOS geometries, console apps and CGA
graphics. CF boot, CGA Tetris and usable game speed have been confirmed on the
real V20-equipped Tandy. The revised Ctrl+A return to the shell needs retesting
there; physical serial-mouse operation is also still unverified.
