# IBM PS/2 real-hardware diagnostic floppy

This branch is for testing ELKS 0.9.2-era behavior on a real 2 MB IBM PS/2
with a 1.44 MB floppy drive and native PS/2 mouse.

The baseline system successfully booted **only after** commenting out
`hma=kernel` in `/bootopts`. A 2026-10-22 hardware test of the diagnostic
FAT floppy confirmed ELKS 0.9.2 boots and `elkfetch` works interactively,
but sysinit printed `KSTACK(7) KERNEL STACK OVERFLOW / Killed` after
displaying the date. Nano-X previously launched with
`MOUSE_PORT=none`, but two verified-working native PS/2 mice froze the
mouse tester or left a stationary graphical mouse cursor. BIOS floppy
read errors also appeared on multiple disks.

## Diagnostic changes (only with CONFIG_IBM_PS2_DIAGNOSTIC=y)

- **Image:** automatically comment out `hma=kernel` while populating
  the diagnostic image, without changing the normal rootfs template.
- **Floppy:** disable track caching and use one-sector BIOS reads on
  floppy drives for a controlled comparison with previously failing
  multi-sector reads. This sacrifices speed to isolate the issue.
- **Welcome screen:** a compact native C `/bin/elkfetch` program prints an
  ELKS ASCII logo, a `user@host` heading with a dashed divider, actual
  `uname()` values (including kernel build string), live uptime and
  RAM/XMS statistics from `/dev/kmem`, the boot-detected CPU class,
  live task count, and root filesystem type. It also reports shell, the
  configured mouse mode, login TTY, and terminal setting. The CPU class
  comes from the boot probe, **not** the generic `uname().machine` target.
  The `MEM_GETSYSINFO` ioctl returns CPU and process counts in four bytes,
  avoiding userland reads of large kernel task structures. The root FS
  comes from `ustatfs(..., UF_NOFREESPACE)` without scanning free FAT
  clusters. CPU/video brand/model, video resolution, and mouse operational
  status are **not** probed by elkfetch. Real-hardware testing
  confirmed auto elkfetch after login; a `KSTACK(6)` startup warning
  remains after the date command. The diagnostic image launches
  `elkfetch` from `/root/.profile` after successful root login, not
  from `/etc/rc.sys`. It does **not** touch mouse hardware.
- **Startup:** diagnostic images add `export MOUSE_TYPE=ps2` to
  `/etc/profile`. The old `rm -f /tmp/nxsock` early boot cleanup
  has been disabled temporarily during KSTACK debugging. If Nano-X leaves
  a stale socket, remove it manually before restarting the desktop.
- **Mouse:** read the existing 8042 command byte and preserve the BIOS's
  settings (including keyboard and translation state). Change only
  the bits needed to enable the auxiliary mouse port and IRQ 12.
  Save/restore the pre-test controller state when the driver is closed.
  Print diagnostic messages before and after controller initialization.
- **Scope:** changes are behind one opt-in config option; upstream ELKS
  defaults and standard IBM images remain unchanged.

The PS/2 mouse and Nano-X have been **confirmed working on physical IBM PS/2 hardware** using the 2026-10-22 archived release. The expanded elkfetch fields and MEM_GETSYSINFO interface are new and still require hardware testing. The startup KSTACK warning is not yet resolved.

## Manual IBM test floppy build (no automatic CI on commits)

Only one workflow creates disk images in this fork:

1. Open [Actions → IBM PS/2 floppy (manual)](https://github.com/ARMoir/elks/actions/workflows/main.yml).
2. Click **Run workflow** and select the default branch (`master`).
   The job itself checks out `ibm-ps2-hardware` source and configuration.
3. When the run succeeds, download the `elks-ibm-ps2-fd1440-fat` artifact.
4. Extract `fd1440.img` (1,474,560 bytes) and write it as a raw image
   to a known-good 1.44 MB floppy.

Commits **do not trigger builds**. The old broad `main` workflow, which
built dozens of floppy and hard-disk variants, was replaced with this
manual one-image pipeline. The redundant diagnostic workflow was removed.
The optional `cross` toolchain CI is also manually triggered.

## Local build on Ubuntu / WSL

Install build prerequisites described in `BUILD.md`, then:

```bash
# From the repository root of ibm-ps2-hardware:
tools/build.sh                 # only if the cross toolchain does not exist
. ./env.sh
make -C elks/arch/i86/drivers/char/KeyMaps config.in
cp ibmpc-1440-ps2diag.config .config
make include/autoconf.h
make -C libc all
make -C libc DESTDIR="$CROSSDIR" install
make kernel bootblocks
make -C elkscmd all
./buildext.sh microwindows
make -C image
# Output: image/fd1440.img, no other disk-image variants
```

## Real-hardware test sequence

1. Boot the diagnostic floppy and verify the `#` shell prompt.
2. Exercise ordinary file reads and note whether BIOS I/O errors remain.
   For example, run `cat /etc/issue` and `ls /bin`.
3. With a native PS/2 mouse connected **before power-on**, enter:

   ```sh
   # MOUSE_TYPE=ps2 is already set by the diagnostic image.
   mouse
   ```

4. Move the mouse and click buttons. If movement coordinates appear,
   press Ctrl+C and try:

   ```sh
   # Only if the previous Nano-X session left a stale socket:
   rm -f /tmp/nxsock
   nxstart
   ```

5. If ELKS freezes, photograph the **last displayed `ps2diag:` line**
   and the most recent `bioshd` messages. Reboot using the normal
   working floppy as needed; never hot-swap the PS/2 mouse.

Expected milestones:

```text
ps2diag: IRQ12 ready, reading 8042 CCB
ps2diag: 8042 CCB=<hex>, enabling mouse
ps2diag: mouse enable finished
```

If initialization never reaches the final line, the controller command
sequence may be hanging. If it reaches the last line but nothing moves,
investigate IRQ 12 routing and 3-byte mouse packets next.

**Do not merge this branch upstream until it builds and the changes are
tested on real hardware.**
