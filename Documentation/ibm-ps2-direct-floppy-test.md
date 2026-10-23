# IBM PS/2 direct floppy controller experiment

This is a **separate test build**, derived from `ibm-ps2-hardware`.
Do not replace the proven-working FAT floppy image until this test boots,
logs in, and runs Nano-X with the physical PS/2 mouse.

## Hypothesis

The PS/2 currently sometimes prints `KSTACK(pid) KERNEL STACK OVERFLOW`
while `/etc/rc.sys` runs. BIOS INT 13h floppy calls can consume part of the
small (700-byte) per-process kernel stack. ELKS also performs FAT filesystem
operations on that stack. Using the **direct FDC** driver for the root filesystem
may avoid BIOS stack consumption during ordinary file reads.

This is a test, not an established diagnosis or confirmed fix.

## Exact changes

Only the diagnostic FAT floppy image produced from this branch adds these
active entries to the end of `/bootopts`:

```ini
root=df0
disable=fd0
```

`root=df0` chooses the direct FDC driver (block major 4).
`disable=fd0` skips probing the BIOS floppy device (block major 3), avoiding
two drivers trying to access the same drive.

Both `CONFIG_BLK_DEV_FD=y` and `CONFIG_ASYNCIO=y` are **already enabled** in
`ibmpc-1440-ps2diag.config`; no kernel size or source driver edits are required.
The original 700-byte kernel stack, HMA workaround, FAT image format, elkfetch,
PS/2 mouse fixes, and Nano-X programs are unchanged. The ELKS bootloader still
uses BIOS to load the kernel; only post-boot root filesystem I/O switches to df0.

The generated `/bootopts` should remain below the 1023-byte limit. CI verifies
both the staging file and the actual contents of the generated floppy image.

## Build and retrieve image

Open [IBM PS/2 direct floppy (manual)](https://github.com/ARMoir/elks/actions/workflows/ps2-direct-floppy.yml).
The separate workflow checks out `ibm-ps2-direct-floppy-test` and builds **one**
1.44 MB FAT image. Download the artifact
`elks-ibm-ps2-direct-fdc-fd1440-fat`, extract `fd1440.img`, and write to a
*different*, clearly marked 1.44 MB floppy. Preserve the original working disk.

## Real hardware test

1. Insert the test floppy and power on; do not hot-plug the PS/2 mouse.
2. Watch for a `fd0: disabled` message and `df: CMOS` detection; direct
   floppy initialization may also announce its FDC type.
3. Confirm boot completes and the login shell starts. Note whether a KSTACK
   error is displayed after the date command.
4. Run `cat /etc/issue`, `ls /bin`, `date`, `elkfetch` and repeat file reads.
5. Test the mouse and `nxstart` as on the already verified PS/2 floppy.
6. Photograph or write down any `df:`, `IRQ 6`, `FDC`, read-error, or
   `KSTACK` messages. If the test freezes or fails, restart using the
   **original** known-good floppy, not this experiment.

If direct FDC refuses to mount the floppy, it may be a controller/IRQ/DMA
compatibility problem. In that case the test has **not** proven a KSTACK fix;
use the existing BIOS-floppy build and investigate the FDC error separately.
