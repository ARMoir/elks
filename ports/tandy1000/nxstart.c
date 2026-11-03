/* Small keyboard/serial-mouse launcher for 640x200 CGA-compatible displays. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <termios.h>
#include "nano-X.h"

static const char *names[] = { "1  Tetris", "2  Mines", "3  Clock", "4  Eyes", "Q  Return to console" };
static const char *paths[] = { "/bin/nxtetris", "/bin/nxmine", "/bin/nxclock", "/bin/nxeyes" };
static GR_WINDOW_ID window;
static GR_GC_ID gc;
static pid_t server_pid, client_pid;
static volatile int stopping;

static void reap(int sig)
{
    signal(SIGCHLD, reap);
    while (waitpid(-1, NULL, WNOHANG) > 0) ;
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
        execl("/bin/nano-x", "nano-x", (char *)NULL);
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

static void launch(int app)
{
    if (app == 4) quit(0);
    if (app < 0 || app > 3 || access(paths[app], F_OK)) return;
    if (vfork() == 0) {
        signal(SIGTERM, SIG_DFL);
        signal(SIGINT, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        execl(paths[app], paths[app], (char *)NULL);
        _exit(127);
    }
}

static void draw(void)
{
    int i;
    GrText(window, gc, 10, 17, "Select an application", -1, GR_TFASCII);
    for (i = 0; i < 5; ++i)
        if (i == 4 || access(paths[i], F_OK) == 0)
            GrText(window, gc, 12, 40 + 18*i, (void *)names[i], -1, GR_TFASCII);
    GrText(window, gc, 10, 137, "Ctrl+A returns to console", -1, GR_TFASCII);
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
        "ELKS CGA Apps", GR_ROOT_WINDOW_ID, (screen.cols - 250)/2, 12,
        250, 148, GR_RGB(255,255,255));
    gc = GrNewGC();
    GrSetGCFont(gc, GrCreateFontEx(GR_FONT_SYSTEM_FIXED, 0, 0, NULL));
    GrSetGCForeground(gc, GR_RGB(0,0,0));
    GrSetGCUseBackground(gc, GR_FALSE);
    GrSelectEvents(window, GR_EVENT_MASK_EXPOSURE | GR_EVENT_MASK_KEY_DOWN |
        GR_EVENT_MASK_BUTTON_DOWN | GR_EVENT_MASK_CLOSE_REQ);
    GrMapWindow(window);
    GrSetFocus(window);
    for (;;) {
        GrGetNextEvent(&event);
        if (event.type == GR_EVENT_TYPE_EXPOSURE) draw();
        else if (event.type == GR_EVENT_TYPE_CLOSE_REQ) quit(0);
        else if (event.type == GR_EVENT_TYPE_KEY_DOWN) {
            int key = event.keystroke.ch;
            if (key == 'q' || key == 'Q' || key == MWKEY_ESCAPE) quit(0);
            if (key >= '1' && key <= '4') launch(key - '1');
        } else if (event.type == GR_EVENT_TYPE_BUTTON_DOWN) {
            GrGetWindowInfo(window, &info);
            GrRaiseWindow(info.parent != GR_ROOT_WINDOW_ID ? info.parent : window);
            GrSetFocus(window);
            if (event.button.y >= 25 && event.button.y < 115)
                launch((event.button.y - 25)/18);
        }
    }
}
