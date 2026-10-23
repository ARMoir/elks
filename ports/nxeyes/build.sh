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

# On ELKS's 1.44 MB floppy the launcher has hard-coded entries for apps
# which might not have been packaged (for example nxmine or nxworld).
# Compact its in-memory menu table at launcher startup using the presence
# of binaries in /bin. This also keeps the fixed menu hit-testing aligned
# with the visible entries, and always keeps the "quit" action.
table_marker = 'typedef struct managed_window mwin;'
if data.count(table_marker) != 1:
    raise SystemExit("nxstart: menu table end marker changed; refusing to patch")
filter_code = r'''
#if ELKS
/* Show only Nano-X applications installed on this particular floppy.
 * Startup-only lookup: no repetitive floppy access during mouse events.
 * Compact in place so labels, click indices, and the menu height agree. */
static void filter_installed_apps(void)
{
    struct app_info *read_app = Apps;
    struct app_info *write_app = Apps;

    for (; read_app->app_id[0] != '\0'; ++read_app) {
        if (strcmp(read_app->app_path, "quit") != 0 &&
            access(read_app->app_path, F_OK) != 0)
            continue;
        if (write_app != read_app)
            *write_app = *read_app;
        ++write_app;
    }
    write_app->app_id[0] = '\0';
    write_app->app_path[0] = '\0';
}
#endif

'''
data = data.replace(table_marker, filter_code + table_marker)

menu_anchor = "\tfor(act = Apps; act->app_id[0] != '\\0'; act++) {"
if data.count(menu_anchor) != 1:
    raise SystemExit("nxstart: menu sizing loop marker changed; refusing to patch")
data = data.replace(menu_anchor,
                    "\t/* Remove missing programs before computing menu dimensions. */\n"
                    "#if ELKS\n"
                    "\tfilter_installed_apps();\n"
                    "#endif\n"
                    + menu_anchor)
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
echo "nxeyes: built with Nano-X; nxstart filters missing GUI apps at launch"
