# IBM PS/2 real-hardware diagnostic floppy

This branch is for testing ELKS 0.9.2-era behavior on a real 2 MB IBM PS/2
with a 1.44 MB floppy drive and native PS/2 mouse.

The baseline system successfully booted **only after** commenting out
`hma=kernel` in `/bootopts`. Nano-X launched with
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
  ELKS ASCII logo, actual `uname()` values, available RAM/XMS statistics
  from `/dev/kmem`, shell, and configured mouse mode. Its invocation is
  appended to `/etc/rc.sys` only for the diagnostic build, after the
  ordinary date display. It does **not** touch mouse hardware.
- **Startup:** diagnostic images add `export MOUSE_TYPE=ps2` to `/etc/profile`
  and `rm -f /tmp/nxsock` to `/etc/rc.sys`. The socket is only
  removed at boot; a running Nano-X session is never disturbed by
  repeated app launches.
- **Mouse:** read the existing 8042 command byte and preserve the BIOS's
  settings (including keyboard and translation state). Change only
  the bits needed to enable the auxiliary mouse port and IRQ 12.
  Save/restore the pre-test controller state when the driver is closed.
  Print diagnostic messages before and after controller initialization.
- **Scope:** changes are behind one opt-in config option; upstream ELKS
  defaults and standard IBM images remain unchanged.

These are **untested experimental changes**, not a confirmed mouse fix.

## Automated test floppy

On the `ibm-ps2-hardware` branch, visit **Actions** and run
**IBM PS/2 hardware diagnostic floppy**, or use the artifact attached to
its successful branch build. Download
`elks-ibm-ps2-diagnostic-fd1440-fat` and write its `fd1440.img`
onto a known-good 1.44 MB floppy using raw image-writing software.
The workflow validates size (1,474,560 bytes) and the HMA workaround.

## Local build on Ubuntu / WSL

Install build prerequisites described in `BUILD.md`, then:

```bash
# From the repository root, after fetching the branch:
tools/build.sh             # only required when cross-toolchain is absent
. ./env.sh
cp ibmpc-1440-ps2diag.config .config
./buildext.sh microwindows # installs Nano-X binaries into extapps/
make all
# Output: image/fd1440.img
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
   # /tmp/nxsock was cleaned at boot; no repeated cleanup required.
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
