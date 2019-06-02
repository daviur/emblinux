// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/serial_reg.h>
#include <linux/of.h>
#include <asm/processor.h>

/* Add your code here */

//#ifdef CONFIG_OF
static const struct of_device_id uart_dt_ids[] = {
	{ .compatible = "bootlin,serial", },
	{ }
};
MODULE_DEVICE_TABLE(of, uart_dt_ids);
//#endif

struct serial_dev {
	void __iomem *regs;
};

static unsigned int reg_read (struct serial_dev * dev, int off) {
	unsigned int * address = (unsigned int *) ((unsigned int) dev->regs + (off * 4));
	return  *address;
}

static unsigned int reg_write (struct serial_dev * dev, unsigned int val, int off) {
	unsigned int * address = (unsigned int *) ((unsigned int) dev->regs + (off * 4));
	*address = val;
	printk("  [reg_write] -- base: %8x \t address: %8x \t val: %8x\n", dev->regs, address, val);
	return 0;
}

static void write_char (struct serial_dev * device, int inp) {

	unsigned int lsr_value;
	do {
		lsr_value = reg_read(device, UART_LSR);
		printk("  lsr & THRE is %08x\n", lsr_value & UART_LSR_THRE);
		cpu_relax();
	} while ( ! (lsr_value & UART_LSR_THRE) );

	reg_write(device, (unsigned int) inp, UART_TX);

	do {
		lsr_value = reg_read(device, UART_LSR);
		printk("  lsr & THRE is %08x\n", lsr_value & UART_LSR_THRE);
		cpu_relax();
	} while ( ! (lsr_value & UART_LSR_THRE) );

	printk ("I wrote char %c and then the LSR was flushed!\n", inp);
}

static int serial_probe(struct platform_device *pdev) {
	struct serial_dev *dev;
	struct resource *res;
	unsigned int baud_divisor, uartclk;


	// Get the resource
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res != NULL)
		printk("I found %x\n", res->start);
	else 
		return (ENXIO);

	// Allocate space for serial_dev
	dev = devm_kzalloc(&pdev->dev, sizeof(struct serial_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	// Map the I/O memory address into system, virtual memory.  
	dev->regs = devm_ioremap_resource(&pdev->dev, res);
	if (!dev->regs) {
		dev_err(&pdev->dev, "Cannot remap registers\n");
		return -ENOMEM;
	}

	// Copy/pasta from the lab
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);

	/* Configure the baud rate to 115200 */
	of_property_read_u32(pdev->dev.of_node, "clock-frequency", &uartclk);
	baud_divisor = uartclk / 16 / 115200;
	printk ("uartclk is %d and baud_divisor is %d\n", uartclk, baud_divisor);
	reg_write(dev, 0x07, UART_OMAP_MDR1);
	reg_write(dev, 0x00, UART_LCR);
	reg_write(dev, UART_LCR_DLAB, UART_LCR);
	reg_write(dev, baud_divisor & 0xff, UART_DLL);
	reg_write(dev, (baud_divisor >> 8) & 0xff, UART_DLM);
	reg_write(dev, UART_LCR_WLEN8, UART_LCR);
	/*
	reg_write(dev, 0x07, 0x20/4);
	reg_write(dev, 0x00, 0xC/4);
	reg_write(dev, 1<<7, 0xC/4);
	reg_write(dev, baud_divisor & 0xff, 0x0);
	reg_write(dev, (baud_divisor >> 8) & 0xff, 0x04/1);
	reg_write(dev, 0x3, 0xC/4);
	*/

	/* Soft reset */
	reg_write(dev, UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, UART_FCR);
	reg_write(dev, 0x00, UART_OMAP_MDR1);

	//printk ("I think MVR has a value of %08x\n", reg_read(dev, 0x50/4));
	//printk ("I think MVR has a value of %08x\n", reg_read(dev, UART_LSR));

	/* Soft reset */
	//reg_write(dev, UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, UART_FCR);
	//reg_write(dev, 0x00, UART_OMAP_MDR1);

      // Writing a character out
	write_char (dev, 'a');
	

	printk("Called %s\n", __func__);

	return 0;
}

static int serial_remove(struct platform_device *pdev)
{

	// Copy/pasta from the lab
	pm_runtime_disable(&pdev->dev);

	pr_info("Called %s\n", __func__);
        return 0;
}

static struct platform_driver serial_driver = {
        .driver = {
                .name = "serial",
                .owner = THIS_MODULE,
		.of_match_table = uart_dt_ids,
        },
        .probe = serial_probe,
        .remove = serial_remove,
};


module_platform_driver(serial_driver);
MODULE_LICENSE("GPL");
