#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/utsname.h>

/* Add your code here */
static char *whom = "Linux user";
module_param(whom, charp, S_IRUSR | S_IWUSR);

static int our_start (void) {
	printk("Hello %s!!! Version: %s Release: %s\n", whom, utsname()->version, utsname()->release);
	return 0;
}

static void our_end (void) {
	printk("Goodbye %s\n", whom);
}

module_init(our_start);
module_exit(our_end);

MODULE_LICENSE("GPL");

