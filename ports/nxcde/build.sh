#!/usr/bin/env bash
# Replace only the Nano-X app launcher with a tiny CDE-style Front Panel.
# Execute after ports/nxeyes/build.sh. The PS/2 stable image stays untouched.
set -euo pipefail
test -n "$TOPDIR"
demos="$TOPDIR/extapps/microwindows/src/demos/nanox"
test -s "$TOPDIR/ports/nxcde/nxstart.c"
test -s "$TOPDIR/extapps/microwindows/src/lib/libnano-X.a"
cp "$TOPDIR/ports/nxcde/nxstart.c" "$demos/nxstart.c"
rm -f "$demos/nxstart.o" "$TOPDIR/extapps/microwindows/src/bin/nxstart"
make -C "$demos" -f Makefile.elks ../../bin/nxstart
test -s "$TOPDIR/extapps/microwindows/src/bin/nxstart"
echo "nxcde: CDE-inspired front panel built as /bin/nxstart"
