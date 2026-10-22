/*
 * Minimal PS/2 mouse driver for ELKS
 *
 * No support for extensions IntelliMouse PS/2 (ImPS/2) and
 * IntelliMouse Explorer PS/2 (IMEX / Explorer).
 *
 * 23 Jan 2026 Original version by Anton Andreev, adapted by Greg Haerr
 */

#include <linuxmt/config.h>
#include <linuxmt/wait.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/sched.h>
#include <linuxmt/errno.h>
#include <linuxmt/ntty.h>
#include <linuxmt/kernel.h>
#include <arch/io.h>
#include <arch/irq.h>

#define IRQ_MOUSE       12          /* mouse interrupt, may conflict with networking */
#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
#define MAX_RETRIES     20000       /* allow slower real 8042 controllers */
#else
#define MAX_RETRIES     60
#endif

/* 8042 ports */
#define DATA            0x60
#define STATUS          0x64
#define COMMAND         0x64

/* 8042 status register bits */
#define OBUF_FULL       0x01        /* output buffer full */
#define IBUF_FULL       0x02        /* input buffer to device full */
#define ABUF_FULL       0x20        /* aux (mouse) output buffer full */

/* 8042 commands using outb() */
#define SET_CTRL_REG    0x60        /* set control register byte in 8042 */
#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
#define GET_CTRL_REG    0x20        /* read the existing 8042 command byte */
#define CCB_MOUSE_IRQ   0x02        /* IRQ12 enabled */
#define CCB_MOUSE_OFF   0x20        /* auxiliary interface disabled */
static unsigned char original_ccb;
#endif
#define SEND_AUX_CMD    0xd4        /* send command to aux device (mouse) */
#define DISABLE_AUX     0xa7        /* disable aux (mouse) port */
#define ENABLE_AUX      0xa8        /* enable aux (mouse) port */

/* preset control register bit values for set_ctrl_reg() */
#define ENABLE_INTS     0x47        /* enable mouse & keyboard interrupts, scan conv */
#define DISABLE_INTS    0x65        /* disable mouse & interrupts, enable keyboard/scan */

/* aux device (mouse) commands for send_aux_cmd() */
#define AUX_ENABLE_DEV  0xf4        /* enable aux device */
#define AUX_DISABLE_DEV 0xf5        /* disable aux device */

static void poll_aux_status(void)
{
    int retries = MAX_RETRIES;

    do {
        unsigned char st = inb_p(STATUS);

        /* Drain any pending output byte (kbd or mouse) */
        if (st & OBUF_FULL) {
			(void)inb_p(DATA);
			continue;
		}

        /* Input buffer clear and no output pending -> ready */
        if (!(st & IBUF_FULL))
            return;
    } while (--retries);
}

#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
/*
 * On IBM PS/2 hardware, don't overwrite the BIOS's entire 8042
 * command byte with constants taken from a PC-compatible emulator.
 * Read and retain it so keyboard IRQ, translation and platform bits
 * survive mouse enable/disable. A bounded wait avoids a permanent
 * spin if the controller does not respond.
 */
static int get_ctrl_reg(unsigned char *value)
{
    int retries = MAX_RETRIES;

    poll_aux_status();
    outb_p(GET_CTRL_REG, COMMAND);
    while (retries-- > 0) {
        if (inb_p(STATUS) & OBUF_FULL) {
            *value = inb_p(DATA);
            return 0;
        }
    }
    return -EIO;
}
#endif

/* set 8042 control register bits */
static void set_ctrl_reg(int cmd)
{
    poll_aux_status();
    outb_p(SET_CTRL_REG, COMMAND);      /* control register byte follows */
    poll_aux_status();
    outb_p(cmd, DATA);                  /* control register bits */
    poll_aux_status();
}

/* send command to mouse */
static void send_aux_cmd(int cmd)
{
    poll_aux_status();
    outb_p(SEND_AUX_CMD, COMMAND);      /* command to aux device (mouse) follows */
    poll_aux_status();
    outb_p(cmd, DATA);                  /* command */
    poll_aux_status();                  /* discard any ACK */
}

static void ps2_mouse_disable(void)
{
    send_aux_cmd(AUX_DISABLE_DEV);      /* stop mouse packets */
#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
    set_ctrl_reg(original_ccb);         /* restore BIOS's keyboard/controller state */
#else
    set_ctrl_reg(DISABLE_INTS);         /* disable mouse interrupts */
#endif
#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
    if (original_ccb & CCB_MOUSE_OFF)
        outb_p(DISABLE_AUX, COMMAND);   /* restore previous disabled state */
#else
    outb_p(DISABLE_AUX, COMMAND);       /* disable aux (mouse) port */
#endif
    poll_aux_status();
}

static void ps2_mouse_enable(void)
{
    poll_aux_status();
    outb_p(ENABLE_AUX, COMMAND);        /* enable aux (mouse) port */
    send_aux_cmd(AUX_ENABLE_DEV);       /* enable mouse (aux device) */
#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
    set_ctrl_reg((original_ccb | CCB_MOUSE_IRQ) & ~CCB_MOUSE_OFF);
#else
    set_ctrl_reg(ENABLE_INTS);          /* enable mouse interrupts */
#endif
}

/* IRQ handler */
static void ps2_irq(int irq, struct pt_regs *regs)
{
    struct ch_queue *q = &ttys[NR_CONSOLES + NR_PTYS + NR_SERIAL].inq;
    unsigned char c;

    /* Read all available mouse bytes */
    while ((inb(STATUS) & (OBUF_FULL | ABUF_FULL)) == (OBUF_FULL | ABUF_FULL)) {
        c = inb(DATA);
        chq_addch_nowakeup(q, c);
    }

    if (q->len)
        wake_up(&q->wait);
}

static int ps2_open(struct tty *tty)
{
    int err, sav_caps;

    if (tty->usecount)          /* exclusive use only */
        return -EBUSY;

    sav_caps = sys_caps;
    sys_caps |= CAP_IRQ8TO15;   /* force-allow IRQ 12 to allow 8086 emulators to work */
    err = request_irq(IRQ_MOUSE, ps2_irq, INT_GENERIC);
    sys_caps = sav_caps;
    if (err) {
        printk("mouse: irq %d not available, error %d\n", IRQ_MOUSE, -err);
        return err;
    }
    err = tty_allocq(tty, MOUSEINQ_SIZE, 0);  /* input only */
    if (err) {
        free_irq(IRQ_MOUSE);
        return err;
    }

#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
    printk("ps2diag: IRQ12 ready, reading 8042 CCB\n");
    err = get_ctrl_reg(&original_ccb);
    if (err) {
        printk("ps2diag: 8042 CCB read failed (%d)\n", err);
        tty_freeq(tty);
        free_irq(IRQ_MOUSE);
        return err;
    }
    printk("ps2diag: 8042 CCB=%x, enabling mouse\n", original_ccb);
#endif
    tty->usecount = 1;
    ps2_mouse_enable();
#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
    printk("ps2diag: mouse enable finished\n");
#endif

    return 0;
}

static void ps2_release(struct tty *tty)
{
    if (--tty->usecount == 0) {
#ifdef CONFIG_IBM_PS2_DIAGNOSTIC
        printk("ps2diag: closing mouse\n");
#endif
        ps2_mouse_disable();
        free_irq(IRQ_MOUSE);
        tty_freeq(tty);
    }
}

static int ps2_write(struct tty *tty)
{
    return 0;   /* mouse is read-only */
}

static int ps2_ioctl(struct tty *tty, int cmd, char *arg)
{
    switch (cmd) {
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
        return 0;
    }
    return -EINVAL;
}

struct tty_ops ps2_mouse_ops = {
    ps2_open,
    ps2_release,
    ps2_write,
    NULL,
    ps2_ioctl,
    NULL
};
