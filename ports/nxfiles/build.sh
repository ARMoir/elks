#!/usr/bin/env bash
# Build 16-bit Nano-X Files from the existing ELKS Microwindows library.
set -euo pipefail
test -n "$TOPDIR"
src="$TOPDIR/extapps/microwindows/src"
demos="$src/demos/nanox"
test -s "$src/lib/libnano-X.a"
cp "$TOPDIR/ports/nxfiles/nxfiles.c" "$demos/nxfiles.c"
cat >> "$demos/Makefile.elks" <<'MAKE'

# Lightweight read-only CDE style file browser for 1.44MB ELKS PS/2.
$(BIN)nxfiles: nxfiles.o $(NXLIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
MAKE
make -C "$demos" -f Makefile.elks ../../bin/nxfiles
test -s "$src/bin/nxfiles"
echo "nxfiles: tiny read-only file browser built"

