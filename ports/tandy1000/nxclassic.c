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

/* Small two-page launcher. Page one keeps the original 1-4 shortcuts.
 * The second page exposes programs that are already installed on the CF
 * image, plus the new compact read-only Files browser.
 */
static const char *labels[2][7] = {
    { "1  Tetris", "2  Mines", "3  Clock", "4  Eyes",
      "M  More applications", "Q  Return to console", NULL },
    { "1  Calculator", "2  Text editor", "3  Files",
      "4  Terminal (test)", "5  File chooser",
      "B  Back to games", "Q  Return to console" }
};
static const char *app_paths[2][5] = {
    { "/bin/nxtetris", "/bin/nxmine", "/bin/nxclock", "/bin/nxeyes", NULL },
    { "/bin/nxcalc", "/bin/nxedit", "/bin/nxfiles", "/bin/nxterm", "/bin/nxselect" }
};
static int menu_page;
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
        execl("/bin/nxold", "nxold", "--client", (char *)NULL);
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

static void launch(int app)
{
    const char *path;
    if (app < 0 || app >= 5) return;
    path = app_paths[menu_page][app];
    if (!path || access(path, F_OK)) return;
    if (vfork() == 0) {
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        execl(path, path, (char *)NULL);
        _exit(127);
    }
}

static void choose(int row)
{
    if (menu_page == 0 && row == 4) {
        menu_page = 1;
        GrClearWindow(window, GR_TRUE);
    } else if (menu_page == 1 && row == 5) {
        menu_page = 0;
        GrClearWindow(window, GR_TRUE);
    } else if ((menu_page == 0 && row == 5) ||
               (menu_page == 1 && row == 6)) {
        quit(0);
    } else if (row >= 0 && row < 5) {
        launch(row);
    }
}

static void draw(void)
{
    int i;
    GrText(window, gc, 10, 14, menu_page ? "More ELKS applications" :
        "Choose a game or app", -1, GR_TFASCII);
    for (i = 0; i < 7 && labels[menu_page][i]; ++i) {
        if (i < 5 && app_paths[menu_page][i] &&
            access(app_paths[menu_page][i], F_OK) != 0)
            continue;
        GrText(window, gc, 12, 31 + 18*i,
               (void *)labels[menu_page][i], -1, GR_TFASCII);
    }
    GrText(window, gc, 10, 158, "Ctrl+A returns to console", -1, GR_TFASCII);
}

int main(int argc, char **argv)
{
    GR_SCREEN_INFO screen;
    GR_EVENT event;
    GR_WINDOW_INFO info;
    if (argc != 2 || strcmp(argv[1], "--client")) return run_session();
    if (setsid() < 0) return 1;
    atexit(cleanup_clients);
    signal(SIGCHLD, reap);
    signal(SIGTERM, quit);
    if (GrOpen() < 0) return 1;
    GrGetScreenInfo(&screen);
    window = GrNewWindowEx(GR_WM_PROPS_APPWINDOW | GR_WM_PROPS_NORESIZE,
        "ELKS Tandy Apps", GR_ROOT_WINDOW_ID, (screen.cols - 250)/2, 2,
        250, 171, GR_RGB(255,255,255));
    gc = GrNewGC();
    GrSetGCFont(gc, GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL));
    GrSetGCForeground(gc, GR_RGB(0,0,0));
    GrSetGCUseBackground(gc, GR_FALSE);
    GrSelectEvents(window, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_KEY_DOWN |
        GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(window);
    GrSetFocus(window);
    for (;;) {
        reap_children();
        GrGetNextEvent(&event);
        if (event.type == GR_EVENT_TYPE_EXPOSURE) draw();
        else if (event.type == GR_EVENT_TYPE_CLOSE_REQ) quit(0);
        else if (event.type == GR_EVENT_TYPE_KEY_DOWN) {
            int key = event.keystroke.ch;
            if (key == 'q' || key == 'Q' || key == MWKEY_ESCAPE) quit(0);
            if ((key == 'm' || key == 'M') && menu_page == 0)
                choose(4);
            else if ((key == 'b' || key == 'B') && menu_page == 1)
                choose(5);
            else if (key >= '1' && key <= '5')
                choose(key - '1');
        } else if (event.type == GR_EVENT_TYPE_BUTTON_DOWN) {
            GrGetWindowInfo(window, &info);
            GrRaiseWindow(info.parent != GR_ROOT_WINDOW_ID ? info.parent : window);
            GrSetFocus(window);
            if (event.button.y >= 17 && event.button.y < 17 + 7*18)
                choose((event.button.y - 17)/18);
        }
    }
}
