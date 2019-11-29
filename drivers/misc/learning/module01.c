#include <linux/module.h>

int init_module(void)
{
	printk("init_module called\n");
	return 0;
}

void cleanup_module(void)
{
	printk("exit_module called\n");
}

MODULE_LICENSE("GPL");
