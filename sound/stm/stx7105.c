/*
 *   STMicrolectronics STx7105 ans STx7106 audio glue driver
 *
 *   Copyright (c) 2005-2011 STMicroelectronics Limited
 *
 *   Author: Pawel Moll <pawel.moll@st.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/stm/soc.h>
#include <sound/core.h>

#include "common.h"


/*
 * Audio glue driver implementation
 */

#define IO_CTRL(base)		((base) + 0x00)
#define PCM_CLK_EN		0
#define PCM_CLK_EN__INPUT	(0 << PCM_CLK_EN)
#define PCM_CLK_EN__OUTPUT	(1 << PCM_CLK_EN)
#define SPDIFHDMI_EN		3
#define SPDIFHDMI_EN__INPUT	(0 << SPDIFHDMI_EN)
#define SPDIFHDMI_EN__OUTPUT	(1 << SPDIFHDMI_EN)
#define PCMPLHDMI_EN		5
#define PCMPLHDMI_EN__INPUT	(0 << PCMPLHDMI_EN)
#define PCMPLHDMI_EN__OUTPUT	(1 << PCMPLHDMI_EN)
#define CLKREC_SEL		9
#define CLKREC_SEL__PCMPLHDMI	(0 << CLKREC_SEL)
#define CLKREC_SEL__SPDIFHDMI	(1 << CLKREC_SEL)
#define CLKREC_SEL__PCMPL1	(2 << CLKREC_SEL)

struct snd_stm_stx7105_glue {
	struct resource *mem_region;
	void *base;

	struct snd_info_entry *proc_entry;

	snd_stm_magic_field;
};

static void snd_stm_stx7105_glue_dump_registers(struct snd_info_entry *entry,
		struct snd_info_buffer *buffer)
{
	struct snd_stm_stx7105_glue *stx7105_glue = entry->private_data;

	BUG_ON(!stx7105_glue);
	BUG_ON(!snd_stm_magic_valid(stx7105_glue));

	snd_iprintf(buffer, "--- snd_stx7105_glue ---\n");
	snd_iprintf(buffer, "IO_CTRL (0x%p) = 0x%08x\n",
			IO_CTRL(stx7105_glue->base),
			readl(IO_CTRL(stx7105_glue->base)));

	snd_iprintf(buffer, "\n");
}


/*
 * Driver functions.
 */

static int snd_stm_stx7105_glue_probe(struct platform_device *pdev)
{
	int result = 0;
	struct snd_stm_stx7105_glue *stx7105_glue;

	dev_dbg(&pdev->dev, "%s()", __func__);

	if (!stm_soc_is_stx7105()) {
		dev_err(&pdev->dev, "Not supported (other than STx7105 or STx7106) "
				    "SOC detected!");
		result = -EINVAL;
		goto error_soc_type;
	}

	/* Register the sound card */
	result = snd_stm_card_register(SND_STM_CARD_TYPE_AUDIO);
	if (result) {
		dev_err(&pdev->dev, "Failed to register ALSA audio card!");
		goto error_card_register;
	}

	stx7105_glue = kzalloc(sizeof(*stx7105_glue), GFP_KERNEL);
	if (!stx7105_glue) {
		dev_err(&pdev->dev, "Can't allocate memory "
				"for a device description!");
		result = -ENOMEM;
		goto error_alloc;
	}
	snd_stm_magic_set(stx7105_glue);

	result = snd_stm_memory_request(pdev, &stx7105_glue->mem_region,
			&stx7105_glue->base);
	if (result < 0) {
		dev_err(&pdev->dev, "Memory region request failed!");
		goto error_memory_request;
	}

	/* Enable audio outputs */
	writel(PCMPLHDMI_EN__OUTPUT | SPDIFHDMI_EN__OUTPUT |
			PCM_CLK_EN__OUTPUT, IO_CTRL(stx7105_glue->base));

	/* Additional procfs info */
	snd_stm_info_register(&stx7105_glue->proc_entry, "stx7105_glue",
			snd_stm_stx7105_glue_dump_registers, stx7105_glue);

	platform_set_drvdata(pdev, stx7105_glue);

	return result;

error_memory_request:
	snd_stm_magic_clear(stx7105_glue);
	kfree(stx7105_glue);
error_alloc:
error_card_register:
error_soc_type:
	return result;
}

static int snd_stm_stx7105_glue_remove(struct platform_device *pdev)
{
	struct snd_stm_stx7105_glue *stx7105_glue = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s()", __func__);

	BUG_ON(!stx7105_glue);
	BUG_ON(!snd_stm_magic_valid(stx7105_glue));

	/* Remove procfs entry */
	snd_stm_info_unregister(stx7105_glue->proc_entry);

	/* Disable audio outputs */
	writel(PCMPLHDMI_EN__INPUT | SPDIFHDMI_EN__INPUT |
			PCM_CLK_EN__INPUT, IO_CTRL(stx7105_glue->base));

	snd_stm_memory_release(stx7105_glue->mem_region, stx7105_glue->base);

	snd_stm_magic_clear(stx7105_glue);
	kfree(stx7105_glue);

	return 0;
}

static struct platform_driver snd_stm_stx7105_glue_driver = {
	.driver.name	= "snd_stx7105_glue",
	.probe		= snd_stm_stx7105_glue_probe,
	.remove		= snd_stm_stx7105_glue_remove,
};


/*
 * Module initialization
 */

static int __init snd_stm_stx7105_init(void)
{
	int result;

	/* Register the platform driver */
	result = platform_driver_register(&snd_stm_stx7105_glue_driver);
	BUG_ON(result);

	return 0;
}

static void __exit snd_stm_stx7105_exit(void)
{
	platform_driver_unregister(&snd_stm_stx7105_glue_driver);
}

MODULE_AUTHOR("Pawel Moll <pawel.moll@st.com>");
MODULE_DESCRIPTION("STMicroelectronics STx7105 and STx7106 audio driver");
MODULE_LICENSE("GPL");

module_init(snd_stm_stx7105_init);
module_exit(snd_stm_stx7105_exit);
