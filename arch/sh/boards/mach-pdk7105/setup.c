/*
 * arch/sh/boards/mach-pdk7105/setup.c
 *
 * Copyright (C) 2008 STMicroelectronics Limited
 * Author: Stuart Menefy (stuart.menefy@st.com)
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 *
 * STMicroelectronics PDK7105-SDK support.
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/stm/pio.h>
#include <linux/stm/platform.h>
#include <linux/stm/stx7105.h>
#include <linux/stm/emi.h>
#include <linux/stm/sysconf.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/stm/nand.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/flash.h>
#include <linux/irq.h>
#include <asm/processor.h>
#include <linux/phy.h>
#include <linux/gpio_keys.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <asm/irq-ilc.h>
#include <asm/irl.h>
#include <asm/io.h>

/*
 * Flash setup depends on whether system is configured as boot-from-NOR
 * (default) or boot-from-NAND.
 *
 * Jumper settings (board v1.2-011):
 *
 * boot-from-      |   NOR                     NAND
 * ---------------------------------------------------------------
 * JE2 (CS routing) |  0 (EMIA->NOR_CS)        1 (EMIA->NAND_CS)
 *                  |    (EMIB->NOR_CS)          (EMIB->NOR_CS)
 *                  |    (EMIC->NAND_CS)         (EMIC->NOR_CS)
 * JE3 (data width) |  0 (16bit)               1 (8bit)
 * JE5 (mode 15)    |  0 (boot NOR)            1 (boot NAND)
 * ---------------------------------------------------------------
 *
 */
#define PDK7105_PIO_PHY_RESET stm_gpio(15, 5)
#define PDK7105_PIO_FLASH_WP stm_gpio(6, 4)

static int ascs[2] __initdata = { 2, 3 };

static void __init pdk7105_setup(char** cmdline_p)
{
	printk("STMicroelectronics PDK7105-SDK board initialisation\n");

	stx7105_early_device_init();
	stx7105_configure_asc(2, &(struct stx7105_asc_config) {
		.routing.asc2 = stx7105_asc2_pio4,
		.hw_flow_control = 1,
		.is_console = 1, });
}

#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
static struct platform_device pdk7105_leds = {
	.name = "leds-gpio",
	.id = 0,
	.dev.platform_data = &(struct gpio_led_platform_data) {
		.num_leds = 2,
		.leds = (struct gpio_led[]) {
			{
				.name = "LD5",
#if defined(CONFIG_LEDS_TRIGGER_HEARTBEAT)
				.default_trigger = "heartbeat",
#endif
				.gpio = stm_gpio(0, 4),
				.active_low = 0,
				.retain_state_suspended = 1,
				.default_state = 0, /* 0:on 1:off 2:keep */
			},
			{
				.name = "LD6",
#if defined(CONFIG_LEDS_TRIGGER_TIMER)
				.default_trigger = "timer",
#endif
				.gpio = stm_gpio(0, 5),
				.active_low = 0,
				.retain_state_suspended = 1,
				.default_state = 1, /* 0:on 1:off 2:keep */
			},
		},
	},
};
#endif /* CONFIG_LEDS_GPIO CONFIG_LEDS_GPIO_MODULE) */

#if defined(CONFIG_KEYBOARD_GPIO)
static struct gpio_keys_button pdk7105_keys[] = {
	{
		.gpio = stm_gpio(0, 1),
		.code = KEY_ENTER,
		.active_low = 1,
		.wakeup = 1,
	},
};

static struct gpio_keys_platform_data pdk7105_key_data = {
	.buttons	= pdk7105_keys,
	.nbuttons	= ARRAY_SIZE(pdk7105_keys),
};

static struct platform_device pdk7105_key_device = {
	.name		= "gpio-keys",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data = &pdk7105_key_data,
	}
};
#endif

static struct stpio_pin *phy_reset_pin;

static int pdk7105_phy_reset(void* bus)
{
/*add by smit*/
#if 1
	gpio_set_value(PDK7105_PIO_PHY_RESET, 1);
#else
	gpio_set_value(PDK7105_PIO_PHY_RESET, 0);
	udelay(100);
	gpio_set_value(PDK7105_PIO_PHY_RESET, 1);
#endif
	return 1;
}

static struct stmmac_mdio_bus_data stmmac_mdio_bus = {
	.bus_id = 0,
	.phy_reset = pdk7105_phy_reset,
	.phy_mask = 0,
};


/*add by smit*/
#if 0
static struct mtd_partition mtd_parts_table[3] = {
	{
		.name = "Boot firmware",
		.size = 0x00040000,
		.offset = 0x00000000,
	}, {
		.name = "Kernel",
		.size = 0x00200000,
		.offset = 0x00040000,
	}, {
		.name = "Root FS",
		.size = MTDPART_SIZ_FULL,
		.offset = 0x00240000,
	}
};
#else
static struct mtd_partition mtd_parts_table[3] = {
	{
		.name = "Boot firmware",
		.size = 0x000c0000,
		.offset = 0x00000000,
	}, {
		.name = "Kernel",
		.size = 0x00300000,
		.offset = 0x000c0000,
	}, {
		.name = "Root FS",
		.size = 0x03c40000,
		.offset = 0x003c0000,
	}
};
#endif

static struct physmap_flash_data pdk7105_physmap_flash_data = {
	.width		= 2,
	.set_vpp	= NULL,
	.nr_parts	= ARRAY_SIZE(mtd_parts_table),
	.parts		= mtd_parts_table
};

static struct platform_device pdk7105_physmap_flash = {
	.name		= "physmap-flash",
	.id		= -1,
	.num_resources	= 1,
	.resource	= (struct resource[]) {
		{
			.start		= 0x00000000,
			.end		= 128*1024*1024 - 1,
			.flags		= IORESOURCE_MEM,
		}
	},
	.dev		= {
		.platform_data	= &pdk7105_physmap_flash_data,
	},
};

/* Configuration for Serial Flash */
static struct mtd_partition serialflash_partitions[] = {
	{
		.name = "SFLASH_1",
		.size = 0x00080000,
		.offset = 0,
	}, {
		.name = "SFLASH_2",
		.size = MTDPART_SIZ_FULL,
		.offset = MTDPART_OFS_NXTBLK,
	},
};

static struct flash_platform_data serialflash_data = {
	.name = "m25p80",
	.parts = serialflash_partitions,
	.nr_parts = ARRAY_SIZE(serialflash_partitions),
	.type = "m25p64",
};

static struct spi_board_info spi_serialflash[] =  {
	{
		.modalias       = "m25p80",
		.bus_num        = 8,
		.chip_select    = stm_gpio(15, 2),
		.max_speed_hz   = 500000,
		.platform_data  = &serialflash_data,
		.mode           = SPI_MODE_3,
	},
};

static struct platform_device spi_pio_device = {
	.name           = "spi_gpio",
	.id             = 8,
	.num_resources  = 0,
	.dev            = {
		.platform_data = &(struct spi_gpio_platform_data) {
			.sck = stm_gpio(15, 0),
			.mosi = stm_gpio(15, 1),
			.miso = stm_gpio(15, 3),
			.num_chipselect = 1,
		},
	},
};
/* Configuration for NAND Flash */
static struct mtd_partition nand_parts[] = {
/*add by smit*/
/*
 *   0M --   1M   1M  Reserved
 *   1M --   5M   4M  KERNEL
 *   5M -- 155M 150M  FS
 * 155M -- 255M 100M  push update FS
 */
#if 1
	{
		.name	= "NAND kernel",
		.offset	= 0x00100000,
		.size 	= 0x00400000
	}, {
		.name	= "NAND Root",
		.offset	= 0x00500000,
		//.size	= 150M
		.size	= 0x09600000
	}, {
		.name	= "NAND Push",
		.offset	= 0x9b00000,
		//.size	= 100M
		.size	= 0x06400000
	},
#else
	{
		.name   = "NAND root",
		.offset = 0,
		.size   = 0x00800000
	}, {
		.name   = "NAND home",
		.offset = MTDPART_OFS_APPEND,
		.size   = MTDPART_SIZ_FULL
	},
#endif
};

static struct stm_nand_bank_data nand_bank_data = {
	/* STM_NAND_EMI data */
	.emi_withinbankoffset   = 0,
	.csn                    = 2,
	.nr_partitions          = ARRAY_SIZE(nand_parts),
	.partitions             = nand_parts,

	.timing_data = &(struct stm_nand_timing_data) {
		.sig_setup      = 50,           /* times in ns */
		.sig_hold       = 50,
		.CE_deassert    = 0,
		.WE_to_RBn      = 100,
		.wr_on          = 10,
		.wr_off         = 40,
		.rd_on          = 10,
		.rd_off         = 40,
		.chip_delay     = 50,           /* in us */
	},
};

/* Platform data for STM_NAND_EMI/FLEX/AFM. (bank# may be updated later) */
static struct platform_device nand_device = {
		.name	           = "stm-nand-flex",
		.dev.platform_data = &(struct stm_plat_nand_flex_data){
		.nr_banks          = 2,
		.banks	           = &nand_bank_data,
		},
};

static struct platform_device *pdk7105_devices[] __initdata = {
#if defined(CONFIG_LEDS_GPIO) || defined(CONFIG_LEDS_GPIO_MODULE)
	&pdk7105_leds,
#endif
	&spi_pio_device,
#if defined(CONFIG_KEYBOARD_GPIO)
	&pdk7105_key_device,
#endif
};

/* PCI configuration */
static struct stm_plat_pci_config  pci_config = {
	.pci_irq = {
		[0] = PCI_PIN_DEFAULT,
		[1] = PCI_PIN_DEFAULT,
		[2] = PCI_PIN_UNUSED,
		[3] = PCI_PIN_UNUSED
	},
	.serr_irq = PCI_PIN_UNUSED,
	.idsel_lo = 30,
	.idsel_hi = 30,
	.req_gnt = {
		[0] = PCI_PIN_DEFAULT,
		[1] = PCI_PIN_UNUSED,
		[2] = PCI_PIN_UNUSED,
		[3] = PCI_PIN_UNUSED
	},
	.pci_clk = 33333333,
	.pci_reset_gpio = stm_gpio(15, 7)
};

int pcibios_map_platform_irq(struct pci_dev *dev, u8 slot, u8 pin)
{
	/* We can use the standard function on this board */
	return  stx7105_pcibios_map_platform_irq(&pci_config, pin);
}

static int __init device_init(void)
{
	u32 bank1_start;
	u32 bank2_start;
	struct sysconf_field *sc;
	u32 boot_mode,i;

	bank1_start = emi_bank_base(0);
	bank2_start = emi_bank_base(1);

	/* Configure FLASH according to boot device mode pins */
	sc = sysconf_claim(SYS_STA, 1, 15, 16, "boot_mode");
	boot_mode = sysconf_read(sc);
	if (boot_mode == 0x0)
	{
		/* Default configuration */
		pr_info("Configuring FLASH for boot-from-NOR\n");
	}
	else if (boot_mode == 0x1)
	{
		/* Swap NOR/NAND banks */
		pr_info("Configuring FLASH for boot-from-NAND\n");
		pdk7105_physmap_flash.resource[0].start = bank1_start;
		pdk7105_physmap_flash.resource[0].end = bank2_start - 1;
		nand_device.id = 0;
	}
	else
	{
		pr_info("Configuring FLASH for boot-from-spi: boot_mode=%d\n",boot_mode);
		bank1_start = emi_bank_base(1);
		bank2_start = emi_bank_base(2);
		pdk7105_physmap_flash.resource[0].start = bank1_start;
		pdk7105_physmap_flash.resource[0].end = bank2_start - 1;
		nand_device.id = 0; /*If your board is 2.0version,set id to 0*/
	}

	stx7105_configure_pci(&pci_config);
	stx7105_configure_sata(0);
	stx7105_configure_pwm(&(struct stx7105_pwm_config) {
		.out0 = stx7105_pwm_out0_pio13_0,
		.out1 = stx7105_pwm_out1_disabled, });
	stx7105_configure_ssc_i2c(1, &(struct stx7105_ssc_config) {
		.routing.ssc1.sclk = stx7105_ssc1_sclk_pio2_5,
		.routing.ssc1.mtsr = stx7105_ssc1_mtsr_pio2_6, });
	stx7105_configure_ssc_i2c(2, &(struct stx7105_ssc_config) {
		.routing.ssc2.sclk = stx7105_ssc2_sclk_pio3_4,
		.routing.ssc2.mtsr = stx7105_ssc2_mtsr_pio3_5, });
	stx7105_configure_ssc_i2c(3, &(struct stx7105_ssc_config) {
		.routing.ssc3.sclk = stx7105_ssc3_sclk_pio3_6,
		.routing.ssc3.mtsr = stx7105_ssc3_mtsr_pio3_7, });

	/*
	 * Note that USB port configuration depends on jumper
	 * settings:
	 *		  PORT 0  SW		PORT 1	SW
	 *		+----------------------------------------
	 * OC	normal	|  4[4]	J5A 2-3		 4[6]	J10A 2-3
	 *	alt	| 12[5]	J5A 1-2		14[6]	J10A 1-2
	 * PWR	normal	|  4[5]	J5B 2-3		 4[7]	J10B 2-3
	 *	alt	| 12[6]	J5B 1-2		14[7]	J10B 1-2
	 */

	stx7105_configure_usb(0, &(struct stx7105_usb_config) {
		.ovrcur_mode = stx7105_usb_ovrcur_active_low,
		.pwr_enabled = 1,
		.routing.usb0.ovrcur = stx7105_usb0_ovrcur_pio4_4,
		.routing.usb0.pwr = stx7105_usb0_pwr_pio4_5, });
	stx7105_configure_usb(1, &(struct stx7105_usb_config) {
		.ovrcur_mode = stx7105_usb_ovrcur_active_low,
		.pwr_enabled = 1,
		.routing.usb1.ovrcur = stx7105_usb1_ovrcur_pio4_6,
		.routing.usb1.pwr = stx7105_usb1_pwr_pio4_7, });

	gpio_request(PDK7105_PIO_PHY_RESET, "eth_phy_reset");
	gpio_direction_output(PDK7105_PIO_PHY_RESET, 1);

	/* gongjia add set pio15_5 to 1 smit*/
	gpio_set_value(PDK7105_PIO_PHY_RESET, 0);
	for(i=0;i<5;i++)
	{
		udelay(20000);
	}
	gpio_set_value(PDK7105_PIO_PHY_RESET, 1);

	stx7105_configure_ethernet(0, &(struct stx7105_ethernet_config) {
		.mode = stx7105_ethernet_mode_mii,
		.ext_clk = 1,
		.phy_bus = 0,
		.phy_addr = -1,
		.mdio_bus_data = &stmmac_mdio_bus,
		});

#if defined(CONFIG_LIRC_SUPPORT)
	stx7105_configure_lirc(&(struct stx7105_lirc_config) {
		.rx_mode = stx7105_lirc_rx_mode_ir,
		.tx_enabled = 1,
		.tx_od_enabled = 1, });
#endif
	//stx7105_configure_audio_pins(3, 1, 1);

	/*
	 * FLASH_WP is shared by NOR and NAND.  However, since MTD NAND has no
	 * concept of WP/VPP, we must permanently enable it
	 */
	gpio_request(PDK7105_PIO_FLASH_WP, "FLASH_WP");
	gpio_direction_output(PDK7105_PIO_FLASH_WP, 1);

#ifdef NAND_USES_FLEX
	stx7105_configure_nand_flex(1, &nand_bank_data, 1);
#endif

	return platform_add_devices(pdk7105_devices, ARRAY_SIZE(pdk7105_devices));
}
arch_initcall(device_init);

static void __iomem *pdk7105_ioport_map(unsigned long port, unsigned int size)
{
	/* However picking somewhere safe isn't as easy as you might think.
	 * I used to use external ROM, but that can cause problems if you are
	 * in the middle of updating Flash. So I'm now using the processor core
	 * version register, which is guaranted to be available, and non-writable.
	 */
	return (void __iomem *)CCN_PVR;
}

static void __init pdk7105_init_irq(void)
{
#ifndef CONFIG_SH_ST_MB705
	/* Configure STEM interrupts as active low. */
	set_irq_type(ILC_EXT_IRQ(1), IRQ_TYPE_LEVEL_LOW);
	set_irq_type(ILC_EXT_IRQ(2), IRQ_TYPE_LEVEL_LOW);
#endif
}

struct sh_machine_vector mv_pdk7105 __initmv = {
	.mv_name		= "pdk7105",
	.mv_setup		= pdk7105_setup,
	.mv_nr_irqs		= NR_IRQS,
	.mv_init_irq		= pdk7105_init_irq,
	.mv_ioport_map		= pdk7105_ioport_map,
};

