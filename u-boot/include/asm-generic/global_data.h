/*
 * Copyright (c) 2012 The Chromium OS Authors.
 * (C) Copyright 2002-2010
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ASM_GENERIC_GBL_DATA_H
#define __ASM_GENERIC_GBL_DATA_H
/*
 * The following data structure is placed in some memory which is
 * available very early after boot (like DPRAM on MPC8xx/MPC82xx, or
 * some locked parts of the data cache) to allow for a minimum set of
 * global variables during system initialization (until we have set
 * up the memory controller so that we can use RAM).
 *
 * Keep it *SMALL* and remember to set GENERATED_GBL_DATA_SIZE > sizeof(gd_t)
 *
 * Each architecture has its own private fields. For now all are private
 */

#ifndef __ASSEMBLY__
#include <membuff.h>
#include <linux/list.h>

/* Never change the sequence of members !!! */
struct pm_ctx {
	unsigned long sp;
	phys_addr_t cpu_resume_addr;
	unsigned long suspend_regs[15];
};

struct pre_serial {
	u32 using_pre_serial;
	u32 enable;
	u32 id;
	u32 baudrate;
	ulong addr;
};

typedef struct global_data {
	bd_t *bd;
	unsigned long flags;
	unsigned int baudrate;
	unsigned long cpu_clk;		/* CPU clock in Hz!		*/
	unsigned long bus_clk;
	/* We cannot bracket this with CONFIG_PCI due to mpc5xxx */
	unsigned long pci_clk;
	unsigned long mem_clk;
#if defined(CONFIG_LCD) || defined(CONFIG_VIDEO)
	unsigned long fb_base;		/* Base address of framebuffer mem */
#endif
#if defined(CONFIG_POST)
	unsigned long post_log_word;	/* Record POST activities */
	unsigned long post_log_res;	/* success of POST test */
	unsigned long post_init_f_time;	/* When post_init_f started */
#endif
#ifdef CONFIG_BOARD_TYPES
	unsigned long board_type;
#endif
	unsigned long have_console;	/* serial_init() was called */
#if CONFIG_IS_ENABLED(PRE_CONSOLE_BUFFER)
	unsigned long precon_buf_idx;	/* Pre-Console buffer index */
#endif
	unsigned long env_addr;		/* Address  of Environment struct */
	unsigned long env_valid;	/* Environment valid? enum env_valid */

	unsigned long ram_top;		/* Top address of RAM used by U-Boot */
	unsigned long ram_top_ext_size;	/* Extend size of RAM top */
	unsigned long relocaddr;	/* Start address of U-Boot in RAM */
	phys_size_t ram_size;		/* RAM size */
	unsigned long mon_len;		/* monitor len */
	unsigned long irq_sp;		/* irq stack pointer */
	unsigned long start_addr_sp;	/* start_addr_stackpointer */
	unsigned long reloc_off;
	struct global_data *new_gd;	/* relocated global data */

#ifdef CONFIG_DM
	struct udevice	*dm_root;	/* Root instance for Driver Model */
	struct udevice	*dm_root_f;	/* Pre-relocation root instance */
	struct list_head uclass_root;	/* Head of core tree */
#endif
#ifdef CONFIG_TIMER
	struct udevice	*timer;		/* Timer instance for Driver Model */
#endif
	const void *fdt_blob;		/* Our device tree, NULL if none */
	void *new_fdt;			/* Relocated FDT */
	unsigned long fdt_size;		/* Space reserved for relocated FDT */
#ifdef CONFIG_OF_LIVE
	struct device_node *of_root;
	struct device_node *of_root_f;  /* U-Boot of-root instance */
#endif
	const void *ufdt_blob;		/* Our U-Boot device tree, NULL if none */
	const void *fdt_blob_kern;	/* Kernel dtb at the tail of u-boot.bin */
	struct jt_funcs *jt;		/* jump table */
	char env_buf[32];		/* buffer for env_get() before reloc. */
#ifdef CONFIG_TRACE
	void		*trace_buff;	/* The trace buffer */
#endif
#if defined(CONFIG_SYS_I2C)
	int		cur_i2c_bus;	/* current used i2c bus */
#endif
#ifdef CONFIG_SYS_I2C_MXC
	void *srdata[10];
#endif
	unsigned int timebase_h;
	unsigned int timebase_l;
#if CONFIG_VAL(SYS_MALLOC_F_LEN)
	unsigned long malloc_base;	/* base address of early malloc() */
	unsigned long malloc_limit;	/* limit address */
	unsigned long malloc_ptr;	/* current address */
#endif
#ifdef CONFIG_PCI
	struct pci_controller *hose;	/* PCI hose for early use */
	phys_addr_t pci_ram_top;	/* top of region accessible to PCI */
#endif
#ifdef CONFIG_PCI_BOOTDELAY
	int pcidelay_done;
#endif
	struct udevice *cur_serial_dev;	/* current serial device */
	struct arch_global_data arch;	/* architecture-specific data */
#ifdef CONFIG_CONSOLE_RECORD
	struct membuff console_out;	/* console output */
	struct membuff console_in;	/* console input */
#endif
#ifdef CONFIG_DM_VIDEO
	ulong video_top;		/* Top of video frame buffer area */
	ulong video_bottom;		/* Bottom of video frame buffer area */
#endif
#ifdef CONFIG_BOOTSTAGE
	struct bootstage_data *bootstage;	/* Bootstage information */
	struct bootstage_data *new_bootstage;	/* Relocated bootstage info */
#endif
	phys_addr_t pm_ctx_phys;

#ifdef CONFIG_BOOTSTAGE_PRINTF_TIMESTAMP
	int new_line;
#endif
	struct pre_serial serial;
	ulong sys_start_tick;		/* For report system start-up time */
	int console_evt;		/* Console event, maybe some hotkey  */
#ifdef CONFIG_LOG
	int log_drop_count;		/* Number of dropped log messages */
	int default_log_level;		/* For devices with no filters */
	struct list_head log_head;	/* List of struct log_device */
#endif
#if CONFIG_IS_ENABLED(FIT_ROLLBACK_PROTECT)
	u32 rollback_index;
#endif
#ifdef CONFIG_PSTORE
	ulong pstore_addr;
	u32 pstore_size;
#endif
} gd_t;
#endif

#ifdef CONFIG_BOARD_TYPES
#define gd_board_type()		gd->board_type
#else
#define gd_board_type()		0
#endif

/*
 * Global Data Flags - the top 16 bits are reserved for arch-specific flags
 */
#define GD_FLG_RELOC		0x00001	/* Code was relocated to RAM	   */
#define GD_FLG_DEVINIT		0x00002	/* Devices have been initialized   */
#define GD_FLG_SILENT		0x00004	/* Silent mode			   */
#define GD_FLG_POSTFAIL		0x00008	/* Critical POST test failed	   */
#define GD_FLG_POSTSTOP		0x00010	/* POST seqeunce aborted	   */
#define GD_FLG_LOGINIT		0x00020	/* Log Buffer has been initialized */
#define GD_FLG_DISABLE_CONSOLE	0x00040	/* Disable console (in & out)	   */
#define GD_FLG_ENV_READY	0x00080	/* Env. imported into hash table   */
#define GD_FLG_SERIAL_READY	0x00100	/* Pre-reloc serial console ready  */
#define GD_FLG_FULL_MALLOC_INIT	0x00200	/* Full malloc() is ready	   */
#define GD_FLG_SPL_INIT		0x00400	/* spl_init() has been called	   */
#define GD_FLG_SKIP_RELOC	0x00800	/* Don't relocate		   */
#define GD_FLG_RECORD		0x01000	/* Record console		   */
#define GD_FLG_ENV_DEFAULT	0x02000 /* Default variable flag	   */
#define GD_FLG_SPL_EARLY_INIT	0x04000 /* Early SPL init is done	   */
#define GD_FLG_LOG_READY	0x08000 /* Log system is ready for use	   */
#define GD_FLG_KDTB_READY	0x10000 /* Kernel dtb is ready for use	   */

#ifdef CONFIG_ARCH_ROCKCHIP
/* BL32 is enabled */
#define GD_FLG_BL32_ENABLED	0x20000
#endif

#endif /* __ASM_GENERIC_GBL_DATA_H */
