// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input-polldev.h>


struct nunchuk_dev {
	struct i2c_client *i2c_client;
};

static const struct i2c_device_id nunchuk_id[] = {
	{ "nunchuk", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, nunchuk_id);

//#ifdef CONFIG_OF
static const struct of_device_id nunchuk_dt_ids[] = {
	{ .compatible = "nintendo,nunchuk", },
	{ }
};
MODULE_DEVICE_TABLE(of, nunchuk_dt_ids);
//#endif

static int nunchuk_read_registers (struct i2c_client *client, char * outp) {

	char buf = 0;
	int ret = i2c_master_send(client, &buf, sizeof(buf));

	msleep(10);

	//printk( "Write 0 to register: %d\n", ret );

	msleep(10);

	//ret = i2c_master_recv(client, outp, sizeof(outp) );
	ret = i2c_master_recv(client, outp, 6 );
	//printk( "Read register -  return value: %d\n", ret );
	//printk( "Read register -  actual value: %012x\n", *outp );
	//printk( "Read register -  actual value: %012d\n", outp[5] );

	return (ret);
}

static void nunchuk_poll (struct input_polled_dev *dev) {

	int ret;
	char rcv[6];
	int zpressed, cpressed;

	struct nunchuk_dev * nunchuk = dev->private;
	struct i2c_client * client = nunchuk->i2c_client;

	ret = nunchuk_read_registers(client, rcv);
	if (ret != 6)
		return;

	zpressed = (rcv[5] & 1 << 0) >> 0;
	cpressed = (rcv[5] & 1 << 1) >> 1;
	

	input_event(dev->input, EV_KEY, BTN_Z, zpressed);
	input_event(dev->input, EV_KEY, BTN_C, cpressed);
	input_sync(dev->input);
}

static int nunchuk_probe(struct i2c_client *client, const struct i2c_device_id *id) {

	struct input_polled_dev * polled_input;
	struct input_dev * input;
	int ret;

	//char rcv[6];

	char buf[2] = {0xf0, 0x55};

	struct nunchuk_dev *nunchuk;

	ret = i2c_master_send(client, buf, 2);

	nunchuk = devm_kzalloc(&client->dev, sizeof(struct nunchuk_dev), GFP_KERNEL);
	if (!nunchuk) {
		return -ENOMEM;
	}

	/* initialize device */
	/* register to a kernel framework */
	//i2c_set_clientdata(client, <private data>);
	
	//printk( "I have found the nunchuk!\n" );

	polled_input = devm_input_allocate_polled_device(&client->dev);
	if (!polled_input) {
		dev_err(&client->dev, "not enough memory for input device\n");
		return -ENOMEM;
	}

	nunchuk->i2c_client = client;
	polled_input->private = nunchuk;

	polled_input->poll = nunchuk_poll;
	polled_input->poll_interval = 50;
	

	input = polled_input->input;
	input->name = "Wii Nunchuk";
	input->id.bustype = BUS_I2C;
	set_bit(EV_KEY, input->evbit);
	set_bit(BTN_C, input->keybit);
	set_bit(BTN_Z, input->keybit);
	ret = input_register_polled_device(polled_input);
	if (ret) {
		dev_err(&client->dev, "could not register input device\n");
		return ret;
	}

	// Initializing the nunchuk
	//printk( "Return value: %d\n", ret );
	//msleep(1);

	//buf[0] = 0xfb;
	//buf[1] = 0x00;
	ret = i2c_master_send(client, buf, 2);
	msleep(1);
	*( (uint16_t*) buf ) = (uint16_t) 0xf055; 
	ret = i2c_master_send(client, buf, 2);
	//printk( "Return value: %d\n", ret );

	//nunchuk_read_registers(client, rcv);

	return 0;
}

static int nunchuk_remove(struct i2c_client *client) {

	//<private data> = i2c_get_clientdata(client);
	/* unregister device from kernel framework */
	/* shut down the device */

	//printk( "Goodbye nunchuk...\n" );
	
	return 0;
}

static struct i2c_driver nunchuk_driver = {
	.probe = nunchuk_probe,
	.remove = nunchuk_remove,
	.id_table = nunchuk_id,
	.driver = {
		.name = "nunchuk_driver_goes_here_Kane",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(nunchuk_dt_ids),
	},
};

module_i2c_driver(nunchuk_driver);

/* Add your code here */

MODULE_LICENSE("GPL");

