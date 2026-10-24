#!/usr/bin/env python3
"""Add direct program launch to upstream ELKS nxterm; fail on source drift."""
from pathlib import Path
import sys

path = Path(sys.argv[1])
text = path.read_text()
if "CDE_TEXTAPPS" in text:
    sys.exit(0)


def replace(old, new):
    global text
    if text.count(old) != 1:
        raise SystemExit("nxterm: upstream marker changed: " + repr(old[:70]))
    text = text.replace(old, new)


replace("char   *startprogram;", """/* CDE_TEXTAPPS: argv is passed directly, never typed into a shell. */
char   *startprogram;
static char **program_argv;
static char *window_title = TITLE;""")
replace('GrError("usage: nxterm [-v5] [command]]\\n");',
        'GrError("usage: nxterm [-v5] [-t title] [-e program [args...]] [command]\\n");')
replace("""        case 'v':
            visualbell = 1;
            argv++;
            break;
""", """        case 'v':
            visualbell = 1;
            argv++;
            break;

        case 't':
            if (strcmp(*argv, "-t") || !argv[1]) usage();
            window_title = argv[1];
            argv += 2;
            break;

        case 'e':
            if (strcmp(*argv, "-e") || !argv[1]) usage();
            program_argv = argv + 1;
            goto options_done;
""")
replace("    if (*argv)\n        startprogram = *argv++;",
        "options_done:\n    if (!program_argv && *argv)\n        startprogram = *argv++;")
replace("w1 = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, TITLE,", 
        "w1 = GrNewWindowEx(GR_WM_PROPS_APPWINDOW, window_title,")

# ELKS has a separate PTY setup path. Do not accidentally patch Linux's.
start = text.index("#if ELKS\nint \nterm_init(void)")
end = text.index("#elif defined(__FreeBSD__)", start)
part = text[start:end]
assert part.count("    pid_t   pid;\n") == 1
part = part.replace("    pid_t   pid;\n", "")  # use global child PID for cleanup
old = "        execv(nargv[0], nargv);\n        exit(1);"
assert part.count(old) == 1
part = part.replace(old, """        if (program_argv)
            execv(program_argv[0], program_argv);
        else
            execv(nargv[0], nargv);
        perror("nxterm: exec");
        _exit(127);""")
old = "    if (!pid) {\n        setsid();"
assert part.count(old) == 1
part = part.replace(old, """    if (!pid) {
        /* The desktop ignores these; applications must not inherit that. */
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        setsid();""")
text = text[:start] + part + text[end:]

replace("""        case GR_EVENT_TYPE_CLOSE_REQ:
            GrClose();
            exit(0);
            break;""", """        case GR_EVENT_TYPE_CLOSE_REQ:
            /* PTY child has its own session; close must terminate it too. */
            if (pid > 0) kill(-pid, SIGHUP);
            GrClose();
            exit(0);
            break;

        case GR_EVENT_TYPE_BUTTON_DOWN:
            GrGetWindowInfo(w1, &wi);
            GrRaiseWindow(wi.parent && wi.parent != GR_ROOT_WINDOW_ID
                          ? wi.parent : w1);
            GrSetFocus(w1);
            break;""")
path.write_text(text)
print("nxterm: direct -e launch, titled windows, click focus and child cleanup")
