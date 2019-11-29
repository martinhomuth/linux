/*
 * Let's start writing the first real driver. A module registers
 * with the IO subsystem of the kernel.
 * There are two ways of registering (see include/linux/fs.h):
 *   legacy:  register_chrdev(unsigned int major, const char *name,
 *                            const struct file_operations *fops);
 *   current: int register_chrdev_region(dev_t, unsigned,
 *                                       const char *);
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>    /* file_operations, register_chrdev */

static struct file_operations fops;
static int major;
#define DRIVER_NAME "TestDriver"

int __init module_initialize(void)
{
	if ((major = register_chrdev(0, DRIVER_NAME, &fops))) {
		pr_info("Registered device with major number %d\n", major);
		return 0;
	}
	return -EIO;  /* register failed, return negative error code */
}

void __exit module_cleanup(void)
{
	unregister_chrdev(major, DRIVER_NAME);
}

module_init(module_initialize);
module_exit(module_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Homuth");
MODULE_DESCRIPTION("First driver module");
