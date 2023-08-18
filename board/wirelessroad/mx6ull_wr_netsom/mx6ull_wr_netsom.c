// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2016 Freescale Semiconductor, Inc.
 */

#include <init.h>
#include <asm/arch/clock.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <fsl_esdhc_imx.h>
#include <linux/bitops.h>
#include <miiphy.h>
#include <net.h>
#include <netdev.h>
#include <usb.h>
#include <usb/ehci-ci.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	gd->ram_size = imx_ddr_size();

	return 0;
}

#define UART_PAD_CTRL  (PAD_CTL_PKE         | PAD_CTL_PUE       | \
			PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED | \
			PAD_CTL_DSE_40ohm   | PAD_CTL_SRE_FAST  | \
			PAD_CTL_HYS)

static iomux_v3_cfg_t const uart1_pads[] = {
	MX6_PAD_UART1_TX_DATA__UART1_DCE_TX | MUX_PAD_CTRL(UART_PAD_CTRL),
	MX6_PAD_UART1_RX_DATA__UART1_DCE_RX | MUX_PAD_CTRL(UART_PAD_CTRL),
};

static void setup_iomux_uart(void)
{
	imx_iomux_v3_setup_multiple_pads(uart1_pads, ARRAY_SIZE(uart1_pads));
}

int board_mmc_get_env_dev(int devno)
{
	return devno;
}

int mmc_map_to_kernel_blk(int devno)
{
	return devno;
}

int board_early_init_f(void)
{
	setup_iomux_uart();

	return 0;
}

#ifdef CONFIG_FEC_MXC
static int setup_fec(void)
{
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;
	int ret;

	/*
	* Use 50MHz anatop loopback REF_CLK1 for ENET1,
	* clear gpr1[13], set gpr1[17].
	*/
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC1_MASK,
			IOMUX_GPR1_FEC1_CLOCK_MUX1_SEL_MASK);
	
	/*
	* Use 50MHz anatop loopbak REF_CLK2 for ENET2,
	* clear gpr1[14], set gpr1[18].
	*/
	clrsetbits_le32(&iomuxc_regs->gpr[1], IOMUX_GPR1_FEC2_MASK,
			IOMUX_GPR1_FEC2_CLOCK_MUX1_SEL_MASK);
	
	ret = enable_fec_anatop_clock(0, ENET_50MHZ);
	if (ret)
		return ret;
	ret = enable_fec_anatop_clock(1, ENET_50MHZ);
	if (ret)
		return ret;

	enable_enet_clk(1);

	return 0;
}

int set_mac_from_chipid(void)
{
	char buf[18];
	//unsigned int ID0 = *((volatile const unsigned int*)0x21BC410);
	unsigned int ID1 = *((volatile const unsigned int*)0x21BC420);

	sprintf(buf, "06:0a:36:%02x:%02x:%02x",
		(unsigned char)(ID1 >> 16),
		(unsigned char)(ID1 >> 8),
		(unsigned char)(ID1 >> 0)
	);
	printf("MAC1: %s\n", buf);

	env_set("ethaddr", buf);

	sprintf(buf, "06:0a:46:%02x:%02x:%02x",
		(unsigned char)(ID1 >> 16),
		(unsigned char)(ID1 >> 8),
		(unsigned char)(ID1 >> 0)
	);
	printf("MAC2: %s\n", buf);
	env_set("eth1addr", buf);
	return 0;
}


int board_phy_config(struct phy_device *phydev)
{
	phy_write(phydev, MDIO_DEVAD_NONE, 0x1f, 0x8190);

	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}
#endif


int board_late_init(void)
{
	set_mac_from_chipid();

	return 0;
}

int board_init(void)
{
	/* Address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;


#ifdef	CONFIG_FEC_MXC
	
	setup_fec();
#endif

	return 0;
}


int checkboard(void)
{
	printf("Board: WirelessRoad NetSOM\n");

	return 0;
}