#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/input-polldev.h>

struct nunchuk_dev {
	struct i2c_client *i2c_client;
};

static int CHECK_ERROR(int ret, struct i2c_client *client){
	if(ret<0){
		dev_err(&client->dev, "CHECK_ERROR: error %d\n", ret);
	}
	return ret;
}

static void write_command(struct i2c_client *client, char addr, char val){
	char data[] = {addr, val};
	CHECK_ERROR(i2c_master_send(client, data, 2), client);
	msleep(1);
}

static void write_byte(struct i2c_client *client, char byte){
	char data[] = {byte};
	CHECK_ERROR(i2c_master_send(client, data, 1), client);
	msleep(1);
}

static int read_bytes(struct i2c_client *client, char * buf, int len){
	int ret;
	
	msleep(10);
	write_byte(client, 0x00);
	msleep(10);
	ret = CHECK_ERROR(i2c_master_recv(client, buf, len), client);
	// buf[ret] = 0x00;
	return ret;
}

static int read_nunchuck_bytes(struct i2c_client *client, char * buf, int len){
	read_bytes(client, buf, len);
	return read_bytes(client, buf, len);	
}

static void print_nunchuck_state(char * buf){
	printk("Z button............ %d\n", (buf[5] & 0x1) != 0);
	printk("C button............ %d\n", (buf[5] & 0x2) != 0);
	printk("Joystick X.......... %d\n", buf[0]);
	printk("Joystick Y.......... %d\n", buf[1]);
	printk("Accelerometer X..... %d\n", buf[2]);
	printk("Accelerometer Y..... %d\n", buf[3]);
	printk("Accelerometer Z..... %d\n", buf[4]);
}

static void nunchuk_poll(struct input_polled_dev *dev){
	char buf[6];
	struct nunchuk_dev *nunchuck = dev->private;
	struct i2c_client *client = nunchuck->i2c_client;
	int ret = read_bytes(client, buf, 6);
	if(ret != 6){
		return;
	}
	input_event(dev->input, EV_KEY, BTN_Z, (buf[5] & 0x1) == 0);
	input_event(dev->input, EV_KEY, BTN_C, (buf[5] & 0x2) == 0);
	input_sync(dev->input);
}

static int nunchuk_probe(struct i2c_client *client, const struct i2c_device_id *id){
	struct input_polled_dev * polled_input;
	struct input_dev * input;
	struct nunchuk_dev *nunchuk;
	int ret;

	polled_input = devm_input_allocate_polled_device(&client->dev);
	if (!polled_input) {
		dev_err(&client->dev, "not enough memory for input device\n");
		return -ENOMEM;
	}
	
	nunchuk = devm_kzalloc(&client->dev, sizeof(struct nunchuk_dev), GFP_KERNEL);
	if (!nunchuk) {
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
	
	write_command(client, 0xf0, 0x52);
	write_command(client, 0xfb, 0x00);
	return 0;
}

static int nunchuk_remove(struct i2c_client *client){
	return 0;
}

static const struct i2c_device_id nunchuk_id[] = {
	{"nunchuck", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, nunchuk_id);

static const struct of_device_id nunchuk_dt_ids[] = {
	{ .compatible = "nintendo,nunchuk", },
	{ }
};
MODULE_DEVICE_TABLE(of, nunchuk_dt_ids);

static struct i2c_driver nunchuk_driver = {
	.probe		= nunchuk_probe,
	.remove		= nunchuk_remove,
	.id_table	= nunchuk_id,
	.driver = {
		.name 	= "nunchuk_v0.1",
		.owner 	= THIS_MODULE,
		.of_match_table = of_match_ptr(nunchuk_dt_ids),
	},
};

module_i2c_driver(nunchuk_driver);


MODULE_LICENSE("GPL");

