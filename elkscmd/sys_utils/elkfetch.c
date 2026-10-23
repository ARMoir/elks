/*
 * elkfetch - tiny system summary for ELKS
 *
 * Displays information available through existing ELKS interfaces.
 * No BIOS mouse probing or direct I/O-port access is performed.
 *
 * Initial implementation for original IBM PS/2 diagnostic boots.
 */
#define __LIBC__
#include <linuxmt/types.h>
#include <linuxmt/mem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

static const char *mouse_mode(void)
{
    const char *type = getenv("MOUSE_TYPE");
    const char *port = getenv("MOUSE_PORT");

    if (port && !strcmp(port, "none"))
        return "disabled (configured)";
    if (type && !strcmp(type, "ps2"))
        return "PS/2 (configured)";
    if (type && !strcmp(type, "ms"))
        return "serial MS (configured)";
    if (type && !strcmp(type, "pc"))
        return "serial PC (configured)";
    if (type && !strcmp(type, "logi"))
        return "serial Logitech (configured)";
    return "default (not probed)";
}

static int read_memory(struct mem_usage *mu)
{
    int fd = open("/dev/kmem", O_RDONLY | O_ALT);
    int ok;

    if (fd < 0)
        return -1;
    ok = ioctl(fd, MEM_GETUSAGE, mu);
    close(fd);
    return ok;
}

int main(void)
{
    struct utsname os;
    struct mem_usage mu;
    int have_os = (uname(&os) == 0);
    int have_mem = (read_memory(&mu) == 0);

    const char *sysname = have_os ? os.sysname : "ELKS";
    const char *release = have_os ? os.release : "unknown";
    const char *hostname = have_os ? os.nodename : "unknown";
    const char *machine = have_os ? os.machine : "unknown";
    const char *shell = getenv("SHELL");

    if (!shell || !*shell)
        shell = "/bin/sh";

    puts("");
    printf("    _____ _     _  __ ____     OS: %s %s\n", sysname, release);
    printf("   | ____| |   | |/ // ___|    Host: %s\n", hostname);
    printf("   |  _| | |   | ' / \\___ \\    Target: %s\n", machine);
    if (have_mem)
        printf("   | |___| |___| . \\  ___) |   RAM: %u/%u KB used\n",
            mu.main_used, mu.main_used + mu.main_free);
    else
        puts("   | |___| |___| . \\  ___) |   RAM: unavailable");
    if (have_mem)
        printf("   |_____|_____|_|\\_\\|____/    XMS: %u/%u KB used\n",
            mu.xms_used, mu.xms_used + mu.xms_free);
    else
        puts("   |_____|_____|_|\\_\\|____/    XMS: unavailable");
    printf("                               Shell: %s\n", shell);
    printf("       Linux for 8086/286      Mouse: %s\n", mouse_mode());
    puts("");
    return 0;
}
