#!/usr/bin/env bash
# Add a small native Nano-X xeyes equivalent after buildext.sh microwindows.
# Run from the ELKS repository root with . ./env.sh already sourced.
set -euo pipefail

test -n "$TOPDIR"
src="$TOPDIR/extapps/microwindows/src"
demos="$src/demos/nanox"

test -s "$src/lib/libnano-X.a"
test -f "$demos/nxstart.c"
test -f "$demos/Makefile.elks"

cp "$TOPDIR/ports/nxeyes/nxeyes.c" "$demos/nxeyes.c"

# The upstream Nano-X launcher already has an eyes menu item for non-ELKS
# builds. Add the corresponding entry only to its ELKS launcher list.
python3 - "$demos/nxstart.c" <<'PY'
import pathlib
import sys
path = pathlib.Path(sys.argv[1])
data = path.read_text()
needle = '\t{"calculator",PATH "nxcalc"},'
if data.count(needle) != 1:
    raise SystemExit("nxeyes: upstream launcher marker changed; refusing to patch")
data = data.replace(needle, needle + '\n\t{"eyes",    PATH "nxeyes"},')
path.write_text(data)
PY

# Link against the same Nano-X library as all other ELKS GUI clients.
cat >> "$demos/Makefile.elks" <<'MAKE'

# IBM PS/2 native XEyes-style test application (no X11 dependencies).
$(BIN)nxeyes: nxeyes.o $(NXLIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
MAKE

# Build the new program and rebuild the launcher containing its menu entry.
make -C "$demos" -f Makefile.elks ../../bin/nxeyes ../../bin/nxstart
test -s "$src/bin/nxeyes"
test -s "$src/bin/nxstart"
echo "nxeyes: built with Nano-X and added to nxstart launcher"
