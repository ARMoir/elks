#ifndef __LINUXMT_MEM_H
#define __LINUXMT_MEM_H

#define MEM_GETTEXTSIZ  2
#define MEM_GETUSAGE    3
#define MEM_GETTASK     4
#define MEM_GETDS       5
#define MEM_GETCS       6
#define MEM_GETHEAP     7
#define MEM_GETUPTIME   8
#define MEM_GETFARTEXT  9
#define MEM_GETMAXTASKS 10
#define MEM_GETJIFFADDR 11
#define MEM_GETSEGALL   12
#define MEM_GETDMSG     13
#define MEM_GETSYSINFO  14  /* boot-detected CPU class and active task count */

struct mem_usage {              /* all in Kbytes */
    unsigned int main_free;
    unsigned int main_used;
    unsigned int xms_free;
    unsigned int xms_used;
};

/* Small kernel-provided snapshot; no task table needs to be copied to userland. */
struct mem_system_info {
    unsigned int cpu_class;  /* CPU_... value from <linuxmt/kernel.h> */
    unsigned int processes;  /* count of allocated task slots */
};

void mm_get_usage(struct mem_usage *mu);

#endif
