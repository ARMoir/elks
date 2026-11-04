/* Small keyboard/serial-mouse launcher for Tandy 320x200 and CGA displays. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <termios.h>
#include "nano-X.h"

static GR_WINDOW_ID window;
static GR_GC_ID gc;
static pid_t server_pid, client_pid;
static volatile int stopping;

/* Defer waitpid until the ordinary event loop. In particular, never reap a
 * child from the signal handler while Nano-X has a client socket operation
 * in progress. A child exit is not a request to terminate the desktop.
 */
static volatile int reap_pending;

static void reap(int sig)
{
    (void)sig;
    reap_pending = 1;
    signal(SIGCHLD, reap);
}

static void reap_children(void)
{
    if (reap_pending) {
        reap_pending = 0;
        while (waitpid(-1, NULL, WNOHANG) > 0)
            ;
    }
}

/* Also runs when the client library exits after losing its server connection. */
static void cleanup_clients(void)
{
    int pid;
    signal(SIGTERM, SIG_IGN);
    signal(SIGCHLD, SIG_DFL);
    kill(-getpid(), SIGTERM);
    do { pid = waitpid(-1, NULL, 0); }
    while (pid > 0 || (pid < 0 && errno == EINTR));
}

static void quit(int sig)
{
    signal(SIGTERM, SIG_IGN);
    exit(0);
}

static void stop_session(int sig)
{
    stopping = 1;
    if (client_pid > 0) {
        kill(-client_pid, SIGTERM);
        kill(client_pid, SIGTERM);
    }
    if (server_pid > 0) kill(server_pid, SIGTERM);
}

/* Keep a parent outside Nano-X's socket calls until both children have exited. */
static int run_session(void)
{
    struct termios saved;
    int have_termios = tcgetattr(0, &saved) == 0;
    int pid, status, tries, result = 1;
    static const char reset[] = "\033[0m\033[?25h\033[H\033[2J";
    static char managed[] = "NANOX_MANAGED=1";

    if (putenv(managed) < 0) return 1;
    signal(SIGINT, stop_session);
    signal(SIGTERM, stop_session);
    signal(SIGHUP, stop_session);
    unlink(GR_NAMED_SOCKET);
    server_pid = vfork();
    if (server_pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        /* A game closing must not cause the Nano-X server to shut down,
         * even if it temporarily becomes the last connected client. The
         * outer session supervisor still terminates this server on exit.
         */
        execl("/bin/nano-x", "nano-x", "-p", (char *)NULL);
        _exit(127);
    }
    if (server_pid < 0) {
        server_pid = 0;
        goto done;
    }
    /* A slow CF card may take time to load the server. Never start a second one. */
    for (tries = 0; tries < 100 && !stopping; ++tries) {
        pid = waitpid(server_pid, &status, WNOHANG);
        if (pid == server_pid) { server_pid = 0; goto done; }
        if (access(GR_NAMED_SOCKET, F_OK) == 0) break;
        usleep(100000L);
    }
    if (tries == 100 || stopping) goto done;
    client_pid = vfork();
    if (client_pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        execl("/bin/nxstart", "nxstart", "--client", (char *)NULL);
        _exit(127);
    }
    if (client_pid < 0) { client_pid = 0; goto done; }
    do { pid = waitpid(-1, &status, 0); } while (pid < 0 && errno == EINTR);
    if (pid == server_pid) server_pid = 0;
    if (pid == client_pid) client_pid = 0;
    if (pid > 0 && WIFEXITED(status)) result = WEXITSTATUS(status);
done:
    stop_session(0);
    while (server_pid > 0 || client_pid > 0) {
        pid = waitpid(-1, NULL, 0);
        if (pid < 0 && errno == EINTR) continue;
        if (pid < 0) break;
        if (pid == server_pid) server_pid = 0;
        if (pid == client_pid) client_pid = 0;
    }
    if (have_termios) tcsetattr(0, TCSANOW, &saved);
    write(1, reset, sizeof(reset) - 1);
    return result;
}


/*
 * ELKS DeskMate, a lightweight Personal DeskMate-inspired shell for the
 * Tandy 1000 EX. This is an original interface, not Tandy DeskMate code.
 *
 * No bitmap resources, external fonts, continuous refresh or animation:
 * V20/CGA hardware is redrawn only after exposure and direct input.
 * The previous simple launcher is preserved as /bin/nxclassic.
 */
#define DM_W 320
#define DM_H 200
#define DM_CATS 5
#define DM_ITEMS 5

#define DM_BLUE       GR_RGB(0, 0, 170)
#define DM_DARKBLUE   GR_RGB(0, 0, 85)
#define DM_CYAN       GR_RGB(0, 170, 170)
#define DM_WHITE      GR_RGB(255, 255, 255)
#define DM_BLACK      GR_RGB(0, 0, 0)
#define DM_LTGRAY     GR_RGB(192, 192, 192)
#define DM_MAGENTA    GR_RGB(170, 0, 170)

enum { ACTION_RUN, ACTION_INFO, ACTION_HELP, ACTION_CONSOLE, ACTION_CLASSIC, ACTION_TEXTGAMES };
typedef struct {
    const char *name;
    const char *exec;
    unsigned char action;
} DMItem;
static const char *tabs[DM_CATS] = { "Programs", "Files", "Tools", "Games", "System" };
static const DMItem menus[DM_CATS][DM_ITEMS] = {
    {{"Text Writer", "/bin/nxedit", ACTION_RUN},
     {"Calculator", "/bin/nxcalc", ACTION_RUN},
     {"File Browser", "/bin/nxfiles", ACTION_RUN},
     {"Terminal", "/bin/nxterm", ACTION_RUN},
     {"File Select", "/bin/nxselect", ACTION_RUN}},
    {{"Browse Files", "/bin/nxfiles", ACTION_RUN},
     {"Text Writer", "/bin/nxedit", ACTION_RUN},
     {"File Select", "/bin/nxselect", ACTION_RUN},
     {"Terminal", "/bin/nxterm", ACTION_RUN},
     {"How to Save", NULL, ACTION_HELP}},
    {{"Calculator", "/bin/nxcalc", ACTION_RUN},
     {"Desk Clock", "/bin/nxclock", ACTION_RUN},
     {"Mouse Eyes", "/bin/nxeyes", ACTION_RUN},
     {"Terminal", "/bin/nxterm", ACTION_RUN},
     {"File Browser", "/bin/nxfiles", ACTION_RUN}},
    {{"Tetris", "/bin/nxtetris", ACTION_RUN},
     {"Mines", "/bin/nxmine", ACTION_RUN},
     {"Mouse Eyes", "/bin/nxeyes", ACTION_RUN},
     {"Desk Clock", "/bin/nxclock", ACTION_RUN},
     {"Console Games", NULL, ACTION_TEXTGAMES}},
    {{"About ELKS", NULL, ACTION_INFO},
     {"Keyboard Help", NULL, ACTION_HELP},
     {"Classic Launcher", NULL, ACTION_CLASSIC},
     {"File Browser", "/bin/nxfiles", ACTION_RUN},
     {"Return to Shell", NULL, ACTION_CONSOLE}}
};
static int category;
static int selected;
static int screen_cols;
static char status_line[54] = "Select a DeskMate accessory";
static const char *mode_name = "TANDY 16";

/* Screen coordinates are relative to the 320x200 undecorated panel. */
static void fill(int x, int y, int w, int h, GR_COLOR c)
{
    GrSetGCForeground(gc, c);
    GrFillRect(window, gc, x, y, w, h);
}
static void line(int x1, int y1, int x2, int y2, GR_COLOR c)
{
    GrSetGCForeground(gc, c);
    GrLine(window, gc, x1, y1, x2, y2);
}
static void text(int x, int y, const char *s, GR_COLOR c)
{
    GrSetGCForeground(gc, c);
    GrText(window, gc, x, y, (void *)s, -1, GR_TFASCII);
}
static void frame(int x, int y, int w, int h, GR_COLOR c)
{
    line(x, y, x+w-1, y, c);
    line(x, y, x, y+h-1, c);
    line(x+w-1, y, x+w-1, y+h-1, c);
    line(x, y+h-1, x+w-1, y+h-1, c);
}
static void copy_status(const char *s)
{
    strncpy(status_line, s, sizeof(status_line)-1);
    status_line[sizeof(status_line)-1] = 0;
}
static void draw_status(void)
{
    fill(0, 177, DM_W, 23, DM_DARKBLUE);
    line(0, 176, DM_W-1, 176, DM_CYAN);
    text(8, 187, status_line, DM_WHITE);
    text(8, 198, "P/F/T/G/S menu   1-5 run   Esc exit", DM_CYAN);
}
static void draw_tabs(void)
{
    int i, x;
    fill(0, 18, DM_W, 19, DM_LTGRAY);
    for (i=0; i<DM_CATS; ++i) {
        x = 4 + 63*i;
        if (category == i) {
            fill(x, 20, 61, 16, DM_CYAN);
            frame(x, 20, 61, 16, DM_DARKBLUE);
            text(x+5, 32, tabs[i], DM_BLACK);
        } else
            text(x+5, 32, tabs[i], DM_BLACK);
    }
}
static void draw_entries(void)
{
    int i, y;
    const DMItem *item;
    fill(8, 42, 201, 129, DM_WHITE);
    frame(8, 42, 201, 129, DM_CYAN);
    fill(9, 43, 199, 17, DM_DARKBLUE);
    text(18, 55, tabs[category], DM_WHITE);
    for (i=0; i<DM_ITEMS; ++i) {
        item = &menus[category][i];
        y = 61 + i*21;
        if (i == selected)
            fill(12, y, 193, 20, DM_BLUE);
        else
            fill(12, y, 193, 20, DM_WHITE);
        {
            char index[4];
            index[0] = '1' + i;
            index[1] = '.';
            index[2] = 0;
            text(19, y+14, index, i == selected ? DM_WHITE : DM_BLACK);
        }
        text(42, y+14, item->name, i == selected ? DM_WHITE : DM_BLACK);
    }
}
static void draw_sidebar(void)
{
    fill(215, 42, 97, 129, DM_LTGRAY);
    frame(215, 42, 97, 129, DM_CYAN);
    fill(216, 43, 95, 17, DM_DARKBLUE);
    text(229, 55, "DESK", DM_WHITE);
    /* A simple notebook/page motif drawn from rectangles, no bitmap. */
    fill(230, 68, 65, 42, DM_WHITE);
    frame(230, 68, 65, 42, DM_BLACK);
    fill(236, 72, 53, 8, DM_CYAN);
    line(238, 85, 285, 85, DM_BLUE);
    line(238, 92, 285, 92, DM_BLUE);
    line(238, 99, 275, 99, DM_BLUE);
    text(233, 127, "ELKS", DM_BLUE);
    text(223, 143, mode_name, DM_BLUE);
    text(223, 158, "V20  /  CF", DM_BLACK);
}
static void draw(void)
{
    fill(0, 0, DM_W, DM_H, DM_BLUE);
    fill(0, 0, DM_W, 18, DM_DARKBLUE);
    text(6, 14, "ELKS DeskMate", DM_WHITE);
    text(207, 14, "TANDY 1000 EX", DM_CYAN);
    draw_tabs();
    draw_entries();
    draw_sidebar();
    draw_status();
}

static void change_category(int to)
{
    if (to<0 || to>=DM_CATS) return;
    category = to;
    selected = 0;
    draw_tabs();
    draw_entries();
    copy_status("1-5 or click to run accessory");
    draw_status();
}
static void launch_selected(void)
{
    const DMItem *item = &menus[category][selected];
    const char *path = item->exec;
    switch (item->action) {
        case ACTION_INFO:
            copy_status("ELKS DeskMate  -  Tandy V20 / Nano-X");
            break;
        case ACTION_HELP:
            copy_status("P/F/T/G/S menu   arrows, Enter run");
            break;
        case ACTION_CLASSIC:
            copy_status("From shell: nxcga tandy mouse pc classic");
            break;
        case ACTION_TEXTGAMES:
            copy_status("Exit to shell: invaders, ttypong, advent");
            break;
        case ACTION_CONSOLE:
            quit(0);
            return;
        case ACTION_RUN:
            if (!path || access(path, F_OK) != 0) {
                copy_status("This application is not installed");
                break;
            }
            if (vfork() == 0) {
                signal(SIGTERM, SIG_DFL);
                signal(SIGINT, SIG_DFL);
                signal(SIGHUP, SIG_DFL);
                signal(SIGCHLD, SIG_DFL);
                execl(path, path, (char *)NULL);
                _exit(127);
            }
            copy_status("Close the app to return to DeskMate");
            break;
    }
    draw_status();
}
static void key_action(int key)
{
    if (key == 'p' || key == 'P') change_category(0);
    else if (key == 'f' || key == 'F') change_category(1);
    else if (key == 't' || key == 'T') change_category(2);
    else if (key == 'g' || key == 'G') change_category(3);
    else if (key == 's' || key == 'S') change_category(4);
    else if (key >= '1' && key <= '5') {
        selected = key-'1';
        draw_entries();
        launch_selected();
    } else if (key == MWKEY_UP) {
        selected = (selected + DM_ITEMS - 1) % DM_ITEMS;
        draw_entries();
    } else if (key == MWKEY_DOWN) {
        selected = (selected + 1) % DM_ITEMS;
        draw_entries();
    } else if (key == MWKEY_ENTER || key == '\r' || key == '\n') {
        launch_selected();
    } else if (key == MWKEY_ESCAPE || key == 'q' || key == 'Q') {
        quit(0);
    }
}
static void mouse_action(int x, int y)
{
    if (y>=19 && y<38) {
        if (x>=4 && x<319) change_category((x-4)/63);
    } else if (x>=12 && x<206 && y>=61 && y<166) {
        selected = (y-61)/21;
        if (selected>=0 && selected<DM_ITEMS) {
            draw_entries();
            launch_selected();
        }
    }
}

int main(int argc, char **argv)
{
    GR_SCREEN_INFO screen;
    GR_EVENT event;
    GR_WINDOW_INFO info;
    const char *mode;

    if (argc != 2 || strcmp(argv[1], "--client")) return run_session();
    if (setsid() < 0) return 1;
    atexit(cleanup_clients);
    signal(SIGCHLD, reap);
    signal(SIGTERM, quit);
    if (GrOpen() < 0) return 1;

    GrGetScreenInfo(&screen);
    screen_cols = screen.cols;
    mode = getenv("TANDY_MODE");
    if (mode && !strcmp(mode, "mono"))
        mode_name = "MONO 2";
    else if (mode && !strcmp(mode, "color"))
        mode_name = "CGA 4";
    if (screen_cols < DM_W || screen.rows < DM_H) {
        GrClose();
        return 1;
    }
    window = GrNewWindowEx(GR_WM_PROPS_NODECORATE | GR_WM_PROPS_NORESIZE,
        "ELKS DeskMate", GR_ROOT_WINDOW_ID, (screen_cols-DM_W)/2, 0,
        DM_W, DM_H, DM_BLUE);
    if (!window) {
        GrClose();
        return 1;
    }
    gc = GrNewGC();
    GrSetGCFont(gc, GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL));
    GrSetGCUseBackground(gc, GR_FALSE);
    GrSelectEvents(window, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_KEY_DOWN |
        GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(window);
    GrSetFocus(window);

    for (;;) {
        reap_children();
        GrGetNextEvent(&event);
        if (event.type == GR_EVENT_TYPE_EXPOSURE) {
            if (event.exposure.wid == window) draw();
        } else if (event.type == GR_EVENT_TYPE_CLOSE_REQ) {
            quit(0);
        } else if (event.type == GR_EVENT_TYPE_KEY_DOWN) {
            if (event.keystroke.wid == window) key_action(event.keystroke.ch);
        } else if (event.type == GR_EVENT_TYPE_BUTTON_DOWN) {
            if (event.button.wid != window) continue;
            GrGetWindowInfo(window, &info);
            GrRaiseWindow(info.parent != GR_ROOT_WINDOW_ID ? info.parent : window);
            GrSetFocus(window);
            mouse_action(event.button.x, event.button.y);
        }
    }
}
