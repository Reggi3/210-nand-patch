/*
 *  linux/drivers/mtd/nand/s5p_phy_init_ext.c
 *
 *  Copyright (C) 2013, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 *
 *
 * helper functions for s5p_nand_mlc.c
 *
 *  
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/usb.h>
#include <linux/usb/hcd.h>

#include <plat/mphy.h>
#include <plat/sdhci.h>

static inline void s5p_ehci_phy_init(int cmd, struct usb_hcd *hcd)
{
	/* charge pump enable */
	s3c_gpio_setpull(S5PV210_ETC2(6), S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(S5PV210_ETC2(6), S3C_GPIO_DRVSTR_2X);

	/* overcurrent flag */
	s3c_gpio_setpull(S5PV210_ETC2(7), S3C_GPIO_PULL_DOWN);
	s5p_gpio_set_drvstr(S5PV210_ETC2(7), S3C_GPIO_DRVSTR_2X);

	/* FriendlyARM's version
	void __iomem *gpio_regs = (void __iomem *) 0xFE500000;
	u32 tmp;

	tmp = readl(gpio_regs + 0x648);
	tmp |= (1 << (6 * 2)) | (1 << (7 * 2));
	writel(tmp, gpio_regs + 0x648);

	tmp = readl(gpio_regs + 0x64c);
	tmp |= (1 << (6 * 2)) | (1 << (7 * 2));
	writel(tmp, gpio_regs + 0x64c);
	*/

	if (!hcd)
		return;

	if (!hcd->regs || ((unsigned int) hcd->regs == 0xFFFFFFBF))
		return;

	/* set AHB burst mode (INSNREG00) */
	if (cmd == PHY_CMD_EHCI) /* INCR4/8/16 burst mode */
		writel(0xF0000, hcd->regs + 0x90);
	else                    /* INCR4/8 bust mode */
		writel(0x70000, hcd->regs + 0x90);
}

static inline void s5p_sdhci_phy_init(struct platform_device *pdev)
{
	struct s3c_sdhci_platdata *pdata;

	if (!pdev)
		return;

	if (pdev->id == 2) {
		pdata = pdev->dev.platform_data;
		pdata->disable_acmd12 = 1;
	}
}

int s5p_phy_init_ext(unsigned int cmd, unsigned long arg, void *p)
{
	if (cmd <= PHY_CMD_EHCI) {
		s5p_ehci_phy_init(cmd, (struct usb_hcd *) p);
	}
	else {
		s5p_sdhci_phy_init((struct platform_device *) arg);
	}
	return 0;
}

