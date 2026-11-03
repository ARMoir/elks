#!/usr/bin/env bash
# Run from a fresh checkout. Build only the Tandy CF image.
set -euo pipefail
. ./env.sh
make -C elks/arch/i86/drivers/char/KeyMaps config.in
cp tandy1000ex-cf16.config .config
make include/autoconf.h
make -C libc all
make -C libc DESTDIR="$CROSSDIR" install
make kernel bootblocks
make -C elkscmd all

# Reuse the same reviewed Nano-X revision as the PS/2 build, with CGA drivers.
mw="$TOPDIR/extapps/microwindows"
mkdir -p "$TOPDIR/extapps"
git clone https://github.com/ghaerr/microwindows.git "$mw"
git -C "$mw" checkout --detach 969eae602a8a5a022ea978da4016f97409d4421e
# Use CGA pixel copies instead of VGA plane registers and retain the packed
# memory-surface format while selecting the two-color display palette.
git -C "$mw" apply "$TOPDIR/ports/tandy1000/microwindows-cga.patch"
# Native Tandy 16-color and standard 4-color/mono graphics share one Nano-X build.
# The graphics mode is selected at runtime, independently of the serial mouse.
cp ports/tandy1000/scr_tandy.c "$mw/src/drivers/scr_cga.c"
cp ports/tandy1000/vgaplan4_tandy.c "$mw/src/drivers/vgaplan4_cga.c"
sh ports/tandy1000/test-nxcga.sh
sed -i '/^CFLAGS += -DCONFIG_HW_VGA=1$/d' "$mw/src/demos/nanox/Makefile.elks"
cp ports/tandy1000/nxstart.c "$mw/src/demos/nanox/nxstart.c"
cp ports/nxeyes/nxeyes.c "$mw/src/demos/nanox/nxeyes.c"
cat >> "$mw/src/demos/nanox/Makefile.elks" <<'MAKE'

$(BIN)nxeyes: nxeyes.o $(NXLIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
MAKE
make -C "$mw/src" -f Makefile.elks SCREEN_CGA=y
make -C "$mw/src/demos/nanox" -f Makefile.elks ../../bin/nxeyes

# Stage hardware-specific defaults without editing the shared rootfs template.
template="$TOPDIR/image/tandy-template"
cp -a elkscmd/rootfs_template "$template"
cp ports/tandy1000/bootopts "$template/bootopts"
cp ports/tandy1000/login.profile "$template/root/.profile"
printf '\n# Tandy: serial mouse is selected with nxcga mouse.\nexport MOUSE_TYPE=ms\nexport MOUSE_PORT=none\n' >> "$template/etc/profile"
# The EX has no AT CMOS RTC. Its serial ports are reserved for mouse/terminal use.
sed -i '/^clock -s /d' "$template/etc/rc.sys"
printf 'id:1:initdefault:\nsi::sysinit:/etc/rc.sys\nt1:1:respawn:/bin/getty /dev/tty1\n' > "$template/etc/inittab"

# Fit the target 31,360-sector card using 31 complete reference cylinders.
# Reserve its initial track for the MBR. Use a 1 KiB-aligned partition size.
make -C image TEMPLATE_DIR="$template" \
    FAT_MKFSOPTS='-T 31184 -s 63 -h 16 -M 512 -d 2 -r 32 -k -N 0 -c 2'
cp ports/tandy1000/nxcga target/bin/nxcga
chmod 755 target/bin/nxcga
mcopy -o -i image/hd.img target/bin/nxcga ::/bin/nxcga
cp ports/tandy1000/README.md target/root/TANDY.TXT
mcopy -o -i image/hd.img target/root/TANDY.TXT ::/root/TANDY.TXT

ia16-elf-as -o image/tandy-mbr.o ports/tandy1000/mbr.S
ia16-elf-ld -T elks/elks-raw.ld -o image/tandy-mbr.bin image/tandy-mbr.o
python3 ports/tandy1000/make-image.py image/hd.img image/tandy-mbr.bin image/elks-tandy1000ex-cf16-smallcard.img

# Verify image contents rather than just the staging directory.
volume='image/elks-tandy1000ex-cf16-smallcard.img@@32256'
mkdir -p image/tandy-verify
for app in sh login getty elkfetch nxcga nxstart nano-x nxeyes nxtetris nxmine nxclock fm kilo invaders ttypong; do
    mcopy -i "$volume" "::/bin/$app" "image/tandy-verify/$app"
    test -s "image/tandy-verify/$app"
done
mcopy -i "$volume" ::/bootopts image/tandy-verify/bootopts
mcopy -i "$volume" ::/root/.profile image/tandy-verify/profile
cmp ports/tandy1000/bootopts image/tandy-verify/bootopts
cmp ports/tandy1000/login.profile image/tandy-verify/profile
cmp ports/tandy1000/nxcga image/tandy-verify/nxcga
mdir -i "$volume" ::/bin
cp ports/tandy1000/README.md image/elks-tandy1000ex-cf16-smallcard-README.txt

# Offer a small update for an already-written card, using the actual packaged
# executables (including their ELKS executable compression).
mkdir -p image/tandy-console-update/bin
cp image/tandy-verify/nxstart image/tandy-verify/nano-x image/tandy-verify/nxcga image/tandy-console-update/bin/
cp ports/tandy1000/console-update.txt image/tandy-console-update/README.txt
