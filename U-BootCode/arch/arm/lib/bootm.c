/* Copyright (C) 2011
 * Corscience GmbH & Co. KG - Simon Schwarz <schwarz@corscience.de>
 *  - Added prep subcommand support
 *  - Reorganized source - modeled after powerpc version
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>
#include <libfdt.h>
#include <mapmem.h>
#include <fdt_support.h>
#include <asm/bootm.h>
#include <asm/secure.h>
#include <linux/compiler.h>
#include <bootm.h>
#include <vxworks.h>

#ifdef CONFIG_ARMV7_NONSEC
#include <asm/armv7.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

static struct tag *params;

static ulong get_sp(void)
{
	ulong ret;

	asm("mov %0, sp" : "=r"(ret) : );
	return ret;
}

void arch_lmb_reserve(struct lmb *lmb)
{
	ulong sp;

	/*
	 * Booting a (Linux) kernel image
	 *
	 * Allocate space for command line and board info - the
	 * address should be as high as possible within the reach of
	 * the kernel (see CONFIG_SYS_BOOTMAPSZ settings), but in unused
	 * memory, which means far enough below the current stack
	 * pointer.
	 */
	sp = get_sp();
	debug("## Current stack ends at 0x%08lx ", sp);

	/* adjust sp by 4K to be safe */
	sp -= 4096;
	lmb_reserve(lmb, sp,
		    gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size - sp);
}

/**
 * announce_and_cleanup() - Print message and prepare for kernel boot
 *
 * @fake: non-zero to do everything except actually boot
 */
static void announce_and_cleanup(int fake)
{
	printf("\nStarting kernel ...%s\n\n", fake ?
		"(fake run for tracing)" : "");
	bootstage_mark_name(BOOTSTAGE_ID_BOOTM_HANDOFF, "start_kernel");
#ifdef CONFIG_BOOTSTAGE_FDT
	bootstage_fdt_add_report();
#endif
#ifdef CONFIG_BOOTSTAGE_REPORT
	bootstage_report();
#endif

#ifdef CONFIG_USB_DEVICE
	udc_disconnect();
#endif
	cleanup_before_linux();		// 做一些清理工作
}

static void setup_start_tag (bd_t *bd)
{
	params = (struct tag *)bd->bi_boot_params;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next (params);
}

static void setup_memory_tags(bd_t *bd)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size (tag_mem32);

		params->u.mem.start = bd->bi_dram[i].start;
		params->u.mem.size = bd->bi_dram[i].size;

		params = tag_next (params);
	}
}

static void setup_commandline_tag(bd_t *bd, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}

static void setup_initrd_tag(bd_t *bd, ulong initrd_start, ulong initrd_end)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size (tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next (params);
}

static void setup_serial_tag(struct tag **tmp)
{
	struct tag *params = *tmp;
	struct tag_serialnr serialnr;

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size (tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high= serialnr.high;
	params = tag_next (params);
	*tmp = params;
}

static void setup_revision_tag(struct tag **in_params)
{
	u32 rev = 0;

	rev = get_board_rev();
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size (tag_revision);
	params->u.revision.rev = rev;
	params = tag_next (params);
}

static void setup_end_tag(bd_t *bd)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

__weak void setup_board_tags(struct tag **in_params) {}

#ifdef CONFIG_ARM64
static void do_nonsec_virt_switch(void)
{
	smp_kick_all_cpus();
	dcache_disable();	/* flush cache before swtiching to EL2 */
	armv8_switch_to_el2();
#ifdef CONFIG_ARMV8_SWITCH_TO_EL1
	armv8_switch_to_el1();
#endif
}
#endif

/* Subcommand: PREP */
static void boot_prep_linux(bootm_headers_t *images)
{
	char *commandline = getenv("bootargs");

	if (IMAGE_ENABLE_OF_LIBFDT && images->ft_len) {
#ifdef CONFIG_OF_LIBFDT
		debug("using: FDT\n");
		if (image_setup_linux(images)) {
			printf("FDT creation failed! hanging...");
			hang();
		}
#endif
	} else if (BOOTM_ENABLE_TAGS) {
		debug("using: ATAGS\n");
		setup_start_tag(gd->bd);
		if (BOOTM_ENABLE_SERIAL_TAG)
			setup_serial_tag(&params);
		if (BOOTM_ENABLE_CMDLINE_TAG)
			setup_commandline_tag(gd->bd, commandline);
		if (BOOTM_ENABLE_REVISION_TAG)
			setup_revision_tag(&params);
		if (BOOTM_ENABLE_MEMORY_TAGS)
			setup_memory_tags(gd->bd);
		if (BOOTM_ENABLE_INITRD_TAG) {
			/*
			 * In boot_ramdisk_high(), it may relocate ramdisk to
			 * a specified location. And set images->initrd_start &
			 * images->initrd_end to relocated ramdisk's start/end
			 * addresses. So use them instead of images->rd_start &
			 * images->rd_end when possible.
			 */
			if (images->initrd_start && images->initrd_end) {
				setup_initrd_tag(gd->bd, images->initrd_start,
						 images->initrd_end);
			} else if (images->rd_start && images->rd_end) {
				setup_initrd_tag(gd->bd, images->rd_start,
						 images->rd_end);
			}
		}
		setup_board_tags(&params);
		setup_end_tag(gd->bd);
	} else {
		printf("FDT and ATAGS support not compiled in - hanging\n");
		hang();
	}
}

#ifdef CONFIG_ARMV7_NONSEC
bool armv7_boot_nonsec(void)
{
	char *s = getenv("bootm_boot_mode");
#ifdef CONFIG_ARMV7_BOOT_SEC_DEFAULT
	bool nonsec = false;
#else
	bool nonsec = true;
#endif

	if (s && !strcmp(s, "sec"))
		nonsec = false;

	if (s && !strcmp(s, "nonsec"))
		nonsec = true;

	return nonsec;
}
#endif

/* Subcommand: GO */
static void boot_jump_linux(bootm_headers_t *images, int flag)
{
#ifdef CONFIG_ARM64			// 64 位 ARM 芯片对应的代码, 而Cortex-A7 是 32 位芯片
	void (*kernel_entry)(void *fdt_addr, void *res0, void *res1,
			void *res2);
	int fake = (flag & BOOTM_STATE_OS_FAKE_GO);

	kernel_entry = (void (*)(void *fdt_addr, void *res0, void *res1,
				void *res2))images->ep;

	debug("## Transferring control to Linux (at address %lx)...\n",
		(ulong) kernel_entry);
	bootstage_mark(BOOTSTAGE_ID_RUN_OS);

	announce_and_cleanup(fake);

	if (!fake) {
		do_nonsec_virt_switch();
		kernel_entry(images->ft_addr, NULL, NULL, NULL);
	}
#else	// machid保存机器ID，如不使用设备树,这个ID会被传递给内核，内核会在自己的ID列表里面查找是否存在与uboot传递进来的machid匹配如果存在就说明 Linux 内核支持这个机器，那么 Linux 就会启动！
	unsigned long machid = gd->bd->bi_arch_number;	// 如果使用设备树的话这个 machid 就无效了，设备树存有一个“兼容性”这个属性，Linux 内核会比较“兼容性”属性的值(字符串)来查看是否支持这个机器。
	char *s;
	void (*kernel_entry)(int zero, int arch, uint params);	// 此函数是进入 Linux 内核的。
	unsigned long r2;
	int fake = (flag & BOOTM_STATE_OS_FAKE_GO);
	// 第一个参数 zero 同样为 0；第二个参数为机器 ID；第三个参数 ATAGS 或者设备树(DTB)首地址，ATAGS 是传统的方法，用于传递一些命令行信息啥的，如果使用设备树的话就要传递设备树(DTB)。
	kernel_entry = (void (*)(int, int, uint))images->ep;
	// 函数 kernel_entry 并不是 uboot 定义的，而是 Linux 内核定义的，Linux 内核镜像文件的第一行代码就是函数 kernel_entry，而 images->ep 保存着 Linux内核镜像的起始地址，起始地址保存的正是 Linux 内核第一行代码
	s = getenv("machid");
	if (s) {
		if (strict_strtoul(s, 16, &machid) < 0) {
			debug("strict_strtoul failed!\n");
			return;
		}
		printf("Using machid 0x%lx from environment\n", machid);
	}

	debug("## Transferring control to Linux (at address %08lx)" \
		"...\n", (ulong) kernel_entry);
	bootstage_mark(BOOTSTAGE_ID_RUN_OS);
	announce_and_cleanup(fake);		// 打印一些信息并做一些清理工作arch/arm/lib/bootm.c
	// 为什么要设置 r2 的值呢?Linux 内核一开始是汇编代码，因此函数 kernel_entry 就是个汇编函数。向汇编函数传递参数要使用 r0、r1 和 r2(参数数量不超过 3 个的时候)，所以 r2 寄存器就是函数 kernel_entry 的第三个参数
	if (IMAGE_ENABLE_OF_LIBFDT && images->ft_len)	
		r2 = (unsigned long)images->ft_addr;	// 如果使用设备树的话，r2 应该是设备树的起始地址，而设备树地址保存在 images的 ftd_addr 成员变量中
	else	// 如果不使用设备树的话，r2 应该是 uboot 传递给 Linux 的参数起始地址，也就是环境变量 bootargs 的值
		r2 = gd->bd->bi_boot_params;

	if (!fake) {
#ifdef CONFIG_ARMV7_NONSEC
		if (armv7_boot_nonsec()) {
			armv7_init_nonsec();
			secure_ram_addr(_do_nonsec_entry)(kernel_entry,
							  0, machid, r2);
		} else
#endif
			kernel_entry(0, machid, r2);	// 调用 kernel_entry 函数进入 Linux 内核，此行将一去不复返，uboot 的使命也就完成了
	}
#endif
}

/* Main Entry point for arm bootm implementation
 *
 * Modeled after the powerpc implementation
 * DIFFERENCE: Instead of calling prep and go at the end
 * they are called if subcommand is equal 0.
 */
int do_bootm_linux(int flag, int argc, char * const argv[],
		   bootm_headers_t *images)
{
	/* No need for those on ARM */
	if (flag & BOOTM_STATE_OS_BD_T || flag & BOOTM_STATE_OS_CMDLINE)
		return -1;

	if (flag & BOOTM_STATE_OS_PREP) {
		boot_prep_linux(images);
		return 0;
	}

	if (flag & (BOOTM_STATE_OS_GO | BOOTM_STATE_OS_FAKE_GO)) {
		boot_jump_linux(images, flag);
		return 0;
	}

	boot_prep_linux(images);
	boot_jump_linux(images, flag);
	return 0;
}

#ifdef CONFIG_CMD_BOOTZ

struct zimage_header {
	uint32_t	code[9];
	uint32_t	zi_magic;
	uint32_t	zi_start;
	uint32_t	zi_end;
};

#define	LINUX_ARM_ZIMAGE_MAGIC	0x016f2818

int bootz_setup(ulong image, ulong *start, ulong *end)
{
	struct zimage_header *zi;

	zi = (struct zimage_header *)map_sysmem(image, 0);	// 设置镜像首地址
	if (zi->zi_magic != LINUX_ARM_ZIMAGE_MAGIC) {
		puts("Bad Linux ARM zImage magic!\n");		// 如果该地址读取不到系统镜像，则报错
		return 1;
	}

	*start = zi->zi_start;
	*end = zi->zi_end;

	printf("Kernel image @ %#08lx [ %#08lx - %#08lx ]\n", image, *start,
	      *end);

	return 0;
}

#endif	/* CONFIG_CMD_BOOTZ */

#if defined(CONFIG_BOOTM_VXWORKS)
void boot_prep_vxworks(bootm_headers_t *images)
{
#if defined(CONFIG_OF_LIBFDT)
	int off;

	if (images->ft_addr) {
		off = fdt_path_offset(images->ft_addr, "/memory");
		if (off < 0) {
			if (arch_fixup_fdt(images->ft_addr))
				puts("## WARNING: fixup memory failed!\n");
		}
	}
#endif
	cleanup_before_linux();
}
void boot_jump_vxworks(bootm_headers_t *images)
{
	/* ARM VxWorks requires device tree physical address to be passed */
	((void (*)(void *))images->ep)(images->ft_addr);
}
#endif
