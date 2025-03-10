/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */





/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <version.h>
#include <SsRecovery.h>

DECLARE_GLOBAL_DATA_PTR;

#undef CONFIG_SSTAR_AUTORUN_DSTAR
#define CONFIG_SSTAR_AUTORUN_DSTAR 1


/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void modem_init(void)
{
#ifdef CONFIG_MODEM_SUPPORT
	debug("DEBUG: main_loop:   gd->do_mdm_init=%lu\n", gd->do_mdm_init);
	if (gd->do_mdm_init) {
		char *str = getenv("mdm_cmd");

		setenv("preboot", str);  /* set or delete definition */
		mdm_init(); /* wait for modem connection */
	}
#endif  /* CONFIG_MODEM_SUPPORT */
}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = getenv("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif

		run_command_list(p, -1, 0);

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}


/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;
	
	//ld@apical ?
	//run_command("gpio out 0 0",0);
	//run_command("gpio out 1 0",0);
	//run_command("gpio out 2 0",0);

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

#ifndef CONFIG_SYS_GENERIC_BOARD
	puts("Warning: Your board does not use generic board. Please read\n");
	puts("doc/README.generic-board and take action. Boards not\n");
	puts("upgraded by the late 2014 may break or be removed.\n");
#endif

	modem_init();
#ifdef CONFIG_VERSION_VARIABLE
	setenv("ver", version_string);  /* set version variable */
#endif /* CONFIG_VERSION_VARIABLE */

	cli_init();

	run_preboot_environment_command();


#ifdef CONFIG_AUTO_UPGRADE_SD
    run_command("sdupgrade", 0);
#endif
#ifdef CONFIG_SSTAR_AUTORUN_DSTAR
    run_command("sdstar", 0);
#endif

#if defined(CONFIG_UPDATE_TFTP)
	update_tftp(0UL);
#endif /* CONFIG_UPDATE_TFTP */

#if defined(CONFIG_CMD_DFU)
extern int update_dfu(void);
	update_dfu();
#endif /* CONFIG_CMD_DFU */

#ifdef CONFIG_MS_EMMC_RECOVERY
	RecoveryCheck();
#endif

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
}
