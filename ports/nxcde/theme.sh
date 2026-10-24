#!/usr/bin/env bash
# Apply CDE color palette at the true NanoWM renderer, not individual apps.
# Run after buildext.sh microwindows (which fetches fresh upstream source).
# Rebuild ONLY modified Nano-X server and nxselect; leave ELKS kernel alone.
set -euo pipefail
test -n "$TOPDIR"
src="$TOPDIR/extapps/microwindows/src"
test -s "$src/nanox/nxdraw.c"
test -s "$src/demos/nanox/nxselect.c"
test -s "$src/bin/nano-x"

python3 - "$src/nanox/nxdraw.c" <<'PY'
import pathlib, re, sys
path = pathlib.Path(sys.argv[1])
text = path.read_text()
start = text.find("#ifdef SCHEME_TAN")
end = text.find("\n#endif", start)
if start < 0 or end < 0:
    raise SystemExit("CDE theme: Microwindows SCHEME_TAN color table changed")
part = text[start:end]
colors = {
    "GR_COLOR_DESKTOP":             (0, 128, 128),    # CDE teal
    "GR_COLOR_ACTIVECAPTION":       (0, 64, 96),      # AIX blue-green title
    "GR_COLOR_ACTIVECAPTIONTEXT":   (255, 255, 255),
    "GR_COLOR_INACTIVECAPTION":     (104, 112, 120),
    "GR_COLOR_INACTIVECAPTIONTEXT": (224, 224, 224),
    "GR_COLOR_3DFRAME":             (0, 0, 0),
    "GR_COLOR_BTNSHADOW":           (96, 96, 96),
    "GR_COLOR_3DLIGHT":             (211, 211, 211),
    "GR_COLOR_BTNHIGHLIGHT":        (255, 255, 255),
    "GR_COLOR_APPWINDOW":           (192, 192, 192),
    "GR_COLOR_APPTEXT":             (0, 0, 0),
    "GR_COLOR_BTNFACE":             (192, 192, 192),
    "GR_COLOR_BTNTEXT":             (0, 0, 0),
    "GR_COLOR_WINDOW":              (255, 255, 255),
    "GR_COLOR_WINDOWTEXT":          (0, 0, 0),
    "GR_COLOR_HIGHLIGHT":           (0, 64, 96),
    "GR_COLOR_HIGHLIGHTTEXT":       (255, 255, 255),
    "GR_COLOR_GRAYTEXT":            (96, 96, 96),
    "GR_COLOR_MENU":                (192, 192, 192),
    "GR_COLOR_MENUTEXT":            (0, 0, 0),
    "GR_COLOR_WINDOWFRAME":         (32, 32, 32),
    "GR_COLOR_WINDOWFRAMELT":       (192, 192, 192),
}
for key, (red, green, blue) in colors.items():
    pattern = r"(?m)^\s*GR_RGB\(\s*\d+,\s*\d+,\s*\d+\),?\s*(?=/\* "+re.escape(key)+r"\s*\*/)"
    matches = list(re.finditer(pattern, part))
    if len(matches) != 1:
        raise SystemExit(f"CDE theme: expected exactly one {key}, got {len(matches)}")
    m = matches[0]
    token = m.group(0)
    comma = "," if token.rstrip().endswith(",") else ""
    replacement = f"\tGR_RGB({red:3}, {green:3}, {blue:3}){comma} "
    part = part[:m.start()] + replacement + part[m.end():]
text = text[:start] + part + text[end:]
path.write_text(text)
print("CDE theme: updated 22 NanoWM server colors")
PY

# Keep the original upstream chooser as /bin/nxselect; add optional
# --browse mode without breaking nxdsktop's path-selection protocol.
cp "$TOPDIR/ports/nxcde/nxselect.c" "$src/demos/nanox/nxselect.c"
make -C "$src" -f Makefile.elks bin/nano-x
make -C "$src/demos/nanox" -f Makefile.elks ../../bin/nxselect
test -s "$src/bin/nano-x"
test -s "$src/bin/nxselect"
echo "CDE theme: NanoWM palettes and nxselect compiled"
