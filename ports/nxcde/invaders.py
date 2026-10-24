#!/usr/bin/env python3
"""Keep ELKS's existing game logic and substitute only the display adapter."""
from pathlib import Path
import sys

source, dest = map(Path, sys.argv[1:])
text = source.read_text()


def replace(old, new):
    global text
    if text.count(old) != 1:
        raise SystemExit("nxinvaders: game source changed: " + repr(old[:70]))
    text = text.replace(old, new)


start = text.index("#ifdef ELKS\n#include \"curses.h\"")
end = text.index("#include <stdio.h>", start)
text = text[:start] + '#include "nxinvaders.h"\n#include <signal.h>\n#define USE_KEYS 1\n' + text[end:]
replace("\ttcsetattr(0, TCSANOW, &save);\n", "")
replace("\ttcgetattr(0, &save);\n", "")
replace("struct termios save;\n", "")
replace("\tstruct termios t;\n", "")
replace("""\ttcgetattr(0, &t);
\tt.c_cc[VMIN] = 0;
\tt.c_cc[VTIME] = 0;
\ttcsetattr(0, TCSANOW, &t);
""", "")
replace("\t\twhile(getch() != ERR);", "\t\twhile(getch() != ERR);\n\t\tnxinvaders_present();")
# Signal cleanup still exits through the adapter's GrClose.
replace("\tsignal(SIGINT, cleanup);", "\tsignal(SIGTERM, cleanup);\n\tsignal(SIGINT, cleanup);")
dest.write_text(text)
