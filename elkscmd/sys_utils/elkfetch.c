/*
 * elkfetch - tiny Neofetch-style system summary for ELKS.
 *
 * Data comes from uname(), libc user/TTY settings, filesystem stats and
 * kernel memory statistics. The kernel reports its boot-probed CPU class
 * and counts tasks; no BIOS calls or hardware probing occur in elkfetch.
 * No mouse device is opened, and no BIOS hardware calls are made.
 */
#define __LIBC__
#include <linuxmt/types.h>
#include <linuxmt/mem.h>
#include <linuxmt/kernel.h>
#include <linuxmt/limits.h>
#include <linuxmt/segment.h>
#include <arch/segment.h>
#include <arch/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <sys/mount.h>

#define INFO_PREFIX "                               "

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

/* Uses the same kernel uptime interface as ELKS's existing 'ps -u'.
 * Unlike wall-clock time, jiffies counts time since boot. */
static int read_uptime(unsigned long *seconds)
{
    int fd = open("/dev/kmem", O_RDONLY | O_ALT);
    unsigned int ds, offset;
    jiff_t ticks;

    if (fd < 0)
        return -1;
    if (ioctl(fd, MEM_GETDS, &ds) < 0 ||
        ioctl(fd, MEM_GETUPTIME, &offset) < 0 ||
        lseek(fd, _MK_LP(ds, offset), SEEK_SET) == -1 ||
        read(fd, &ticks, sizeof(ticks)) != sizeof(ticks)) {
        close(fd);
        return -1;
    }

    close(fd);
    *seconds = ticks / HZ;
    return 0;
}

/* This one 4-byte ioctl returns boot-detected CPU type and live tasks
 * without transferring task_struct records into tiny ELKS user stacks. */
static int read_system_info(struct mem_system_info *info)
{
    int fd = open("/dev/kmem", O_RDONLY | O_ALT);
    int rc;

    if (fd < 0)
        return -1;
    rc = ioctl(fd, MEM_GETSYSINFO, info);
    close(fd);
    return rc;
}

static const char *cpu_name(unsigned int cpu)
{
    switch (cpu) {
    case CPU_8088: return "8088";
    case CPU_8086: return "8086";
    case CPU_NECV20: return "NEC V20";
    case CPU_NECV30: return "NEC V30";
    case CPU_80188: return "80188";
    case CPU_80186: return "80186";
    case CPU_80286: return "80286";
    case CPU_80386: return "80386+";
    default: return "unknown x86";
    }
}

/* Similar to 'mount' with UF_NOFREESPACE: no FAT free-space walk. */
static const char *root_filesystem(void)
{
    struct statfs fs;
    unsigned int i;
    for (i = 0; i < NR_SUPER; ++i) {
        if (ustatfs(i, &fs, UF_NOFREESPACE) == 0 &&
            !strcmp(fs.f_mntonname, "/")) {
            switch (fs.f_type) {
            case FST_MSDOS: return "FAT (MS-DOS)";
            case FST_MINIX: return "MINIX";
            case FST_ROMFS: return "ROMFS";
            default: return "other";
            }
        }
    }
    return "unavailable";
}

int main(void)
{
    struct utsname os;
    struct mem_usage mu;
    struct mem_system_info system_info;
    struct passwd *pw;
    char build_version[35];
    const char *username;
    const char *sysname;
    const char *release;
    const char *hostname;
    const char *machine;
    const char *shell;
    const char *tty;
    const char *term;
    unsigned long seconds;
    unsigned int days, hours, minutes, i, header_len;
    int have_os = (uname(&os) == 0);
    int have_mem = (read_memory(&mu) == 0);
    int have_uptime = (read_uptime(&seconds) == 0);
    int have_system = (read_system_info(&system_info) == 0);

    pw = getpwuid(getuid());
    username = pw && pw->pw_name ? pw->pw_name : getenv("USER");
    if (!username || !*username)
        username = "user";

    sysname = have_os ? os.sysname : "ELKS";
    release = have_os ? os.release : "unknown";
    hostname = have_os ? os.nodename : "unknown";
    machine = have_os ? os.machine : "unknown";
    shell = getenv("SHELL");
    tty = ttyname(0);
    term = getenv("TERM");

    if (!shell || !*shell)
        shell = "/bin/sh";
    if (!tty || !*tty)
        tty = "unknown";
    if (!term || !*term)
        term = "unknown";

    /* Keep the kernel build line inside a narrow 80-column console. */
    strncpy(build_version, have_os ? os.version : "unavailable",
        sizeof(build_version) - 1);
    build_version[sizeof(build_version) - 1] = '\0';

    header_len = strlen(username) + strlen(hostname) + 1;
    if (header_len > 39)
        header_len = 39;

    puts("");
    printf("    _____ _     _  __ ____     %s@%s\n", username, hostname);
    printf("   | ____| |   | |/ // ___|    ");
    for (i = 0; i < header_len; i++)
        putchar('-');
    putchar('\n');
    printf("   |  _| | |   | ' / \\___ \\    OS: %s %s\n", sysname, release);
    printf("   | |___| |___| . \\  ___) |   Target: %s\n", machine);

    if (have_uptime) {
        days = seconds / 86400UL;
        seconds %= 86400UL;
        hours = seconds / 3600UL;
        minutes = (seconds % 3600UL) / 60UL;
        printf("   |_____|_____|_|\\_\\|____/    Uptime: %ud %uh %um\n",
            days, hours, minutes);
    } else {
        puts("   |_____|_____|_|\\_\\|____/    Uptime: unavailable");
    }

    printf(INFO_PREFIX "Kernel build: %s\n", build_version);
    if (have_system)
        printf(INFO_PREFIX "CPU (probe): %s\n", cpu_name(system_info.cpu_class));
    else
        puts(INFO_PREFIX "CPU (probe): unavailable");
    if (have_system)
        printf(INFO_PREFIX "Processes: %u\n", system_info.processes);
    else
        puts(INFO_PREFIX "Processes: unavailable");
    printf(INFO_PREFIX "Root FS: %s\n", root_filesystem());
    if (have_mem)
        printf(INFO_PREFIX "RAM: %u/%u KB used\n",
            mu.main_used, mu.main_used + mu.main_free);
    else
        puts(INFO_PREFIX "RAM: unavailable");
    if (have_mem)
        printf("       Linux for 8086/286      XMS: %u/%u KB used\n",
            mu.xms_used, mu.xms_used + mu.xms_free);
    else
        puts("       Linux for 8086/286      XMS: unavailable");

    printf(INFO_PREFIX "Shell: %s\n", shell);
    printf(INFO_PREFIX "Mouse: %s\n", mouse_mode());
    printf(INFO_PREFIX "TTY: %s\n", tty);
    printf(INFO_PREFIX "Terminal: %s\n", term);
    puts("");
    return 0;
}
