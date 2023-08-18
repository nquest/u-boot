/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2023 WirelessRoad
 * Copyright (C) 2023 Sergey Moryakov <sergey@nqnet.org>
 *
 * Configuration settings for the WirekessRoad i.MX6ULL NetSOM module.
 */

#ifndef __MX6ULLWRNETSOM_CONFIG_H
#define __MX6ULLWRNETSOM_CONFIG_H

#include <linux/sizes.h>
#include "mx6_common.h"


#define PHYS_SDRAM_SIZE	SZ_256M

#define CONFIG_MXC_UART_BASE		UART1_BASE

/* MMC Configs */
#ifdef CONFIG_FSL_USDHC
#define CONFIG_SYS_FSL_ESDHC_ADDR	USDHC2_BASE_ADDR

#define CONFIG_SYS_FSL_USDHC_NUM	1
#endif

/* Physical Memory Map */
#define PHYS_SDRAM			MMDC0_ARB_BASE_ADDR

#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM
#define CONFIG_SYS_INIT_RAM_ADDR	IRAM_BASE_ADDR
#define CONFIG_SYS_INIT_RAM_SIZE	IRAM_SIZE

#define CONFIG_SYS_INIT_SP_OFFSET \
	(CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR	\
	(CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

// #ifdef CONFIG_CMD_NET
// #define IMX_FEC_BASE			ENET_BASE_ADDR
// #define CONFIG_FEC_MXC_PHYADDR		0x0
// #define CONFIG_FEC_XCV_TYPE		RMII
// #define CONFIG_ETHPRIME			"eth0"
// #endif




#if !defined(CONFIG_CMD_SF)
#define CONFIG_ENV_IS_IN_MMC
#else
#define CONFIG_ENV_IS_IN_SPI_FLASH
#endif

#if defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(6 * 64 * 1024)
#define CONFIG_SYS_MMC_ENV_DEV	0
#elif defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_ENV_OFFSET		(768 * 1024)
#define CONFIG_ENV_SECT_SIZE	(8 * 1024)
#define CONFIG_ENV_SPI_BUS		CONFIG_SF_DEFAULT_BUS
#define CONFIG_ENV_SPI_CS		CONFIG_SF_DEFAULT_CS
#define CONFIG_ENV_SPI_MODE		CONFIG_SF_DEFAULT_MODE
#define CONFIG_ENV_SPI_MAX_HZ	CONFIG_SF_DEFAULT_SPEED
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"autoload=0\0" \
	"bootfile=wr.bin\0" \
	"bootcmd=setenv bootargs \"\"; "\
		"sf probe;"\
		"sf read ${loadaddr} 0x100000 0x700000;"\
		"bootm ${loadaddr};"\
		"\0" \
	"console=ttymxc0,115200n8\0" \
	"fdt_addr_r=0x82000000\0" \
	"fdt_high=0xffffffff\0" \
	"initrd_high=0xffffffff\0" \
	"kernel_addr_r=0x81000000\0" \
	"pxefile_addr_r=0x87100000\0" \
	"ramdisk_addr_r=0x82100000\0" \
	"scriptaddr=0x87000000\0" \
	"do_tftp_flash_firmware=dhcp;" \
	 	"tftp ${loadaddr} wr.bin;" \
		"sf probe;" \
		"sf update ${loadaddr} 0 ${filesize};" \
		"\0" \
	"do_tftp_flash_uboot=dhcp;" \
	 	"tftp ${loadaddr} uboot-imx.bin;" \
		"sf probe;" \
		"sf update ${loadaddr} 0 ${filesize};" \
		"\0"

#endif