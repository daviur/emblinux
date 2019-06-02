#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/pm_runtime.h>
#include <linux/serial_reg.h>
#include <linux/of.h>
#include <asm/processor.h>
#include <linux/delay.h>


struct serial_dev {
	void __iomem *regs;
};

static unsigned int reg_read(struct serial_dev *dev, int off){
	mb();
	return *(unsigned int *)((unsigned int)(dev->regs) + 4*off);
}


static void reg_write(struct serial_dev *dev,unsigned int val, int off){
	mb();
	*(unsigned int *)((unsigned int)(dev->regs) + 4*off) = val;
}

static void write_char(struct serial_dev *dev, char in_char){
	while((reg_read(dev, UART_LSR) & UART_LSR_THRE) == 0){
		cpu_relax();
	}
	printk("@Write char: LSR = 0x%x, SYSC = 0x%xm EFR = 0x%x\n", reg_read(dev, UART_LSR), reg_read(dev, UART_OMAP_SYSC), reg_read(dev, UART_EFR));
	reg_write(dev, (unsigned int) in_char, UART_TX);
	msleep(50);
	printk("@Write char: LSR = 0x%x, SYSC = 0x%x\n", reg_read(dev, UART_LSR), reg_read(dev, UART_OMAP_SYSC));
}

static int serial_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct serial_dev *dev;
	unsigned int baud_divisor, uartclk;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENXIO;
	
	dev = devm_kzalloc(&pdev->dev, sizeof(struct serial_dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	
	dev->regs = devm_ioremap_resource(&pdev->dev, res);
	if (!dev->regs) {
		dev_err(&pdev->dev, "Cannot remap registers\n");
		return -ENOMEM;
	}
	
	// pdev->private = dev;
	pm_runtime_enable(&pdev->dev);
	pm_runtime_get_sync(&pdev->dev);
	
	// Soft Reset	
	reg_write(dev, 0x2, UART_OMAP_SYSC);
	while(reg_read(dev, UART_OMAP_SYSS) == 0){
		cpu_relax();
	}

	/* Configure the baud rate to 115200 */
	of_property_read_u32(pdev->dev.of_node, "clock-frequency",
			&uartclk);
	baud_divisor = uartclk / 16 / 115200;
	reg_write(dev, 0x07, UART_OMAP_MDR1);
	reg_write(dev, 0x00, UART_LCR);
	reg_write(dev, UART_LCR_DLAB, UART_LCR);
	reg_write(dev, baud_divisor & 0xff, UART_DLL);
	reg_write(dev, (baud_divisor >> 8) & 0xff, UART_DLM);
	reg_write(dev, UART_LCR_WLEN8, UART_LCR);

	/* Soft reset */
	reg_write(dev, UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT, UART_FCR);
	reg_write(dev, 0x00, UART_OMAP_MDR1);

	write_char(dev, 'a');
	printk("Serial_probe complete; addr %x\n", res->start);
	return 0;
}

static int serial_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);
	printk("Called serial_remove\n");
        return 0;
}


static const struct of_device_id uart_dt_ids[] = {
	{ .compatible = "bootlin,serial", },
	{ }
};
MODULE_DEVICE_TABLE(of, uart_dt_ids);

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
