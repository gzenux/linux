#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/sh_timer.h>
#include <linux/sh_intc.h>
#include <linux/stm/platform.h>

static struct sh_timer_config tmu0_platform_data = {
	.channels_mask = 7,
};

static struct resource tmu0_resources[] = {
	DEFINE_RES_MEM(0xffd80000, 0x30),
	DEFINE_RES_IRQ(evt2irq(0x400)),
	DEFINE_RES_IRQ(evt2irq(0x420)),
	DEFINE_RES_IRQ(evt2irq(0x440)),
};

static struct platform_device tmu0_device = {
	.name		= "sh-tmu",
	.id		= 0,
	.dev = {
		.platform_data	= &tmu0_platform_data,
	},
	.resource	= tmu0_resources,
	.num_resources	= ARRAY_SIZE(tmu0_resources),
};


static struct platform_device *stm_tmu_devices[] __initdata = {
	&tmu0_device,
};

static int __init stm_tmu_devices_setup(void)
{
	return platform_add_devices(stm_tmu_devices,
				    ARRAY_SIZE(stm_tmu_devices));
}
arch_initcall(stm_tmu_devices_setup);

static struct platform_device *stm_tmu_devices_early_devices[] __initdata = {
	&tmu0_device,
};

void __init plat_early_device_setup(void)
{
	early_platform_add_devices(stm_tmu_devices_early_devices,
				   ARRAY_SIZE(stm_tmu_devices_early_devices));
}
