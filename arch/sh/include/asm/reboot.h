#ifndef __ASM_SH_REBOOT_H
#define __ASM_SH_REBOOT_H

#include <linux/kdebug.h>

struct pt_regs;

struct machine_ops {
	void (*restart)(char *cmd);
	void (*halt)(void);
	void (*power_off)(void);
	void (*shutdown)(void);
	void (*crash_shutdown)(struct pt_regs *);
};

extern struct machine_ops machine_ops;

struct restart_prep_handler {
	struct list_head list;
	void (*prepare_restart)(void);
};

/* arch/sh/kernel/machine_kexec.c */
void native_machine_crash_shutdown(struct pt_regs *regs);

/* Register a function to be run immediately before a restart. */
void register_prepare_restart_handler(void (*prepare_restart)(void));

#endif /* __ASM_SH_REBOOT_H */
