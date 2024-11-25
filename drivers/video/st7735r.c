// SPDX-License-Identifier: GPL-2.0+
/*
 * U-Boot driver for the ST7735R LCD Controller
 * Copyright (C) 2024 Sergey Moryakov
 * Copyright (C) 2024 Wireless Road
 *
 * Based on linux fbtft driver
 * Copyright (C) 2013 Noralf Tronnes
 */

#include <common.h>
#include <command.h>
#include <cpu_func.h>
#include <dm.h>
#include <errno.h>
#include <spi.h>
#include <video.h>
#include <asm/gpio.h>
#include <dm/device_compat.h>
#include <linux/delay.h>

#define WIDTH		160
#define HEIGHT		128

#define DEFAULT_GAMMA   "0F 1A 0F 18 2F 28 20 22 1F 1B 23 37 00 07 02 10 0F 1B 0F 17 33 2C 29 2E 30 30 39 3F 00 07 03 10"

/* Commands */
#define ST7735_SOFT_RESET           0x01
#define ST7735_EXIT_SLEEP_MODE      0x11
#define ST7735_ENTER_NORMAL_MODE    0x13
#define ST7735_EXIT_INVERT_MODE     0x20
#define ST7735_DISPLAY_ON           0x29
#define ST7735_SET_PIXEL_FORMAT     0x3A
#define ST7735_FRMCTR1              0xB1
#define ST7735_FRMCTR2              0xB2
#define ST7735_FRMCTR3              0xB3
#define ST7735_INVCTR               0xB4
#define ST7735_PWRCTR1              0xC0
#define ST7735_PWRCTR2              0xC1
#define ST7735_PWRCTR3              0xC2
#define ST7735_PWRCTR4              0xC3
#define ST7735_PWRCTR5              0xC4
#define ST7735_VMCTR1               0xC5
#define ST7735_CASET                0x2A
#define ST7735_RASET                0x2B
#define ST7735_RAMWR                0x2C
#define ST7735_MADCTL               0x36



// orientation and color code
#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04
#define MY BIT(7)
#define MX BIT(6)
#define MV BIT(5)
struct st7735r_priv {
	struct gpio_desc reset_gpio;
	struct gpio_desc dc_gpio;
	struct udevice *dev;
};

static int st7735r_spi_write_cmd(struct udevice *dev, u32 reg)
{
	struct st7735r_priv *priv = dev_get_priv(dev);
	u8 buf8 = reg;
	int ret;

	ret = dm_gpio_set_value(&priv->dc_gpio, 0);
	if (ret) {
		dev_dbg(dev, "Failed to handle dc\n");
		return ret;
	}

	ret = dm_spi_xfer(dev, 8, &buf8, NULL, SPI_XFER_BEGIN | SPI_XFER_END);
	if (ret)
		dev_dbg(dev, "Failed to write command\n");

	return ret;
}

static int st7735r_spi_write_data(struct udevice *dev, u32 val)
{
	struct st7735r_priv *priv = dev_get_priv(dev);
	u8 buf8 = val;
	int ret;

	ret = dm_gpio_set_value(&priv->dc_gpio, 1);
	if (ret) {
		dev_dbg(dev, "Failed to handle dc\n");
		return ret;
	}

	ret = dm_spi_xfer(dev, 8, &buf8, NULL, SPI_XFER_BEGIN | SPI_XFER_END);
	if (ret)
		dev_dbg(dev, "Failed to write data\n");

	return ret;
}

static int st7735r_display_init(struct udevice *dev)
{
    /* Soft Reset */
    (void)st7735r_spi_write_cmd(dev, ST7735_SOFT_RESET);
    mdelay(120);

    /* Exit sleep mode */
    (void)st7735r_spi_write_cmd(dev,ST7735_EXIT_SLEEP_MODE);
    mdelay(10);

    /* FRMCTR1 - frame rate control: normal mode
	 * frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_FRMCTR1);
	(void)st7735r_spi_write_data(dev, 0x01);
	(void)st7735r_spi_write_data(dev, 0x2C);
	(void)st7735r_spi_write_data(dev, 0x2D);


	/* FRMCTR2 - frame rate control: idle mode
	 * frame rate = fosc / (1 x 2 + 40) * (LINE + 2C + 2D)
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_FRMCTR2);
	(void)st7735r_spi_write_data(dev, 0x01);
	(void)st7735r_spi_write_data(dev, 0x2C);
	(void)st7735r_spi_write_data(dev, 0x2D);

	/* FRMCTR3 - frame rate control - partial mode
	 * dot inversion mode, line inversion mode
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_FRMCTR3);
	(void)st7735r_spi_write_data(dev, 0x01);
	(void)st7735r_spi_write_data(dev, 0x2C);
	(void)st7735r_spi_write_data(dev, 0x2D);
	(void)st7735r_spi_write_data(dev, 0x01);
	(void)st7735r_spi_write_data(dev, 0x2C);
	(void)st7735r_spi_write_data(dev, 0x2D);

	/* INVCTR - display inversion control
	 * no inversion
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_INVCTR);
	(void)st7735r_spi_write_data(dev, 0x07);

	/* PWCTR1 - Power Control
	 * -4.6V, AUTO mode
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_PWRCTR1);
	(void)st7735r_spi_write_data(dev, 0xA2);
	(void)st7735r_spi_write_data(dev, 0x02);
	(void)st7735r_spi_write_data(dev, 0x84);


	/* PWCTR2 - Power Control
	 * VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_PWRCTR2);
	(void)st7735r_spi_write_data(dev, 0xC5);

	/* PWCTR3 - Power Control
	 * Opamp current small, Boost frequency
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_PWRCTR3);
	(void)st7735r_spi_write_data(dev, 0x0A);
	(void)st7735r_spi_write_data(dev, 0x00);

	/* PWCTR4 - Power Control
	 * BCLK/2, Opamp current small & Medium low
	 */
	(void)st7735r_spi_write_cmd(dev,ST7735_PWRCTR4);
	(void)st7735r_spi_write_data(dev, 0x8A);
	(void)st7735r_spi_write_data(dev, 0x2A);

	/* PWCTR5 - Power Control */
	// -1, 0xC4, 0x8A, 0xEE,
	(void)st7735r_spi_write_cmd(dev,ST7735_PWRCTR5);
	(void)st7735r_spi_write_data(dev, 0x8A);
	(void)st7735r_spi_write_data(dev, 0xEE);


	/* VMCTR1 - Power Control */
	// -1, 0xC5, 0x0E,
	(void)st7735r_spi_write_cmd(dev,ST7735_VMCTR1);
	(void)st7735r_spi_write_data(dev, 0x0E);

    (void)st7735r_spi_write_cmd(dev, ST7735_EXIT_INVERT_MODE);
	(void)st7735r_spi_write_cmd(dev, ST7735_SET_PIXEL_FORMAT);
	(void)st7735r_spi_write_data(dev, 0x05); // 16 bit pixel format)

	// TODO Set gamma
	(void)st7735r_spi_write_cmd(dev, ST7735_ENTER_NORMAL_MODE);
	mdelay(10);
	(void)st7735r_spi_write_cmd(dev, ST7735_DISPLAY_ON);
	mdelay(50);
	st7735r_spi_write_cmd(dev, ST7735_MADCTL);
	st7735r_spi_write_data(dev, (MY | MV));
	return 0;
}

static int st7735r_bind(struct udevice *dev)
{
	struct video_uc_plat *plat = dev_get_uclass_plat(dev);

	plat->size = WIDTH * HEIGHT * 16;
	dev_info(dev, "Setting FB size to %d", plat->size);

	return 0;
}

static int st7735r_sync(struct udevice *vid)
{
	struct video_priv *uc_priv = dev_get_uclass_priv(vid);
	struct st7735r_priv *priv = dev_get_priv(vid);
	struct udevice *dev = priv->dev;
	int i, ret;
	u8 data1, data2;
	u8 *start = uc_priv->fb;

	ret = dm_spi_claim_bus(dev);
	if (ret) {
		dev_err(dev, "Failed to claim SPI bus: %d\n", ret);
		return ret;
	}

	st7735r_spi_write_cmd(dev, ST7735_CASET);
	st7735r_spi_write_data(dev, 0x0);
	st7735r_spi_write_data(dev, 0x0);
	st7735r_spi_write_data(dev, 0x0);
	st7735r_spi_write_data(dev, 160);
	st7735r_spi_write_cmd(dev, ST7735_RAMWR);
	st7735r_spi_write_data(dev, 0x0);
	st7735r_spi_write_data(dev, 0x0);
	st7735r_spi_write_data(dev, 0x0);
	st7735r_spi_write_data(dev, 128);


	for (i = 0; i < (uc_priv->xsize * uc_priv->ysize); i++) {
		data2 = *start++;
		data1 = *start++;
		(void)st7735r_spi_write_data(dev, data1);
		(void)st7735r_spi_write_data(dev, data2);
	}


// TODO
    dm_spi_release_bus(dev);

	return 0;
};

static int st7735r_spi_startup(struct udevice *dev)
{
	int ret;

	ret = dm_spi_claim_bus(dev);
	if (ret) {
		dev_err(dev, "Failed to claim SPI bus: %d\n", ret);
		return ret;
	}

	ret = st7735r_display_init(dev);
	if (ret)
		return ret;

	dm_spi_release_bus(dev);

	return 0;
}

static int st7735r_probe(struct udevice *dev)
{
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct st7735r_priv *priv = dev_get_priv(dev);
	u32 buswidth;
	int ret;

	buswidth = dev_read_u32_default(dev, "buswidth", 0);
	if (buswidth != 8) {
		dev_err(dev, "Only 8bit buswidth is supported now");
		return -EINVAL;
	}

	// This implementation totally ignores vendor recomendations, so disapling reset
	// Goot luck!
	//
	// ret = gpio_request_by_name(dev, "reset-gpios", 0,
	// 			   &priv->reset_gpio, GPIOD_IS_OUT);
	// if (ret) {
	// 	dev_warn(dev, "missing reset GPIO\n");
	// }

	ret = gpio_request_by_name(dev, "dc-gpios", 0,
				   &priv->dc_gpio, GPIOD_IS_OUT);
	if (ret) {
		dev_err(dev, "missing dc GPIO\n");
		return ret;
	}

	uc_priv->bpix = VIDEO_BPP16;
	uc_priv->xsize = WIDTH;
	uc_priv->ysize = HEIGHT;
	uc_priv->rot = 0;

	priv->dev = dev;

	ret = st7735r_spi_startup(dev);
	if (ret)
		return ret;

	return 0;
};

static const struct video_ops st7735r_ops = {
	.video_sync = st7735r_sync,
};

static const struct udevice_id st7735r_ids[] = {
	{ .compatible = "sitronix,st7735r" },
	{ }
};

U_BOOT_DRIVER(st7735r_video) = {
	.name = "st7735r_video",
	.id = UCLASS_VIDEO,
	.of_match = st7735r_ids,
	.ops = &st7735r_ops,
	.plat_auto = sizeof(struct video_uc_plat),
	.bind = st7735r_bind,
	.probe = st7735r_probe,
	.priv_auto = sizeof(struct st7735r_priv),
};

// /*
//  * Gamma string format:
//  * VRF0P VOS0P PK0P PK1P PK2P PK3P PK4P PK5P PK6P PK7P PK8P PK9P SELV0P SELV1P SELV62P SELV63P
//  * VRF0N VOS0N PK0N PK1N PK2N PK3N PK4N PK5N PK6N PK7N PK8N PK9N SELV0N SELV1N SELV62N SELV63N
//  */
// #define CURVE(num, idx)  curves[(num) * par->gamma.num_values + (idx)]
// static int set_gamma(struct fbtft_par *par, u32 *curves)
// {
// 	int i, j;

// 	/* apply mask */
// 	for (i = 0; i < par->gamma.num_curves; i++)
// 		for (j = 0; j < par->gamma.num_values; j++)
// 			CURVE(i, j) &= 0x3f;

// 	for (i = 0; i < par->gamma.num_curves; i++)
// 		write_reg(par, 0xE0 + i,
// 			  CURVE(i, 0),  CURVE(i, 1),
// 			  CURVE(i, 2),  CURVE(i, 3),
// 			  CURVE(i, 4),  CURVE(i, 5),
// 			  CURVE(i, 6),  CURVE(i, 7),
// 			  CURVE(i, 8),  CURVE(i, 9),
// 			  CURVE(i, 10), CURVE(i, 11),
// 			  CURVE(i, 12), CURVE(i, 13),
// 			  CURVE(i, 14), CURVE(i, 15));

// 	return 0;
// }
