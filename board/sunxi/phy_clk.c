#include <common.h>
#include <netdev.h>
#include <miiphy.h>
#include <asm/io.h>
#include <asm/arch/clock.h>

void eth_init_board(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* Set EPHY 25Mhz clock */
	setbits_le32(&ccm->emac_25m_clk_cfg, CCM_EMAC_25M_SRC_ENABLE);
	setbits_le32(&ccm->emac_25m_clk_cfg, CCM_EMAC_25M_ENABLE);
	printf("Enabling EMAC 25M Clock\n");
}
