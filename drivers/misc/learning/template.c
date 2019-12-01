#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

static dev_t dev_num;
static struct cdev *device_object;
static struct class *class;
static struct device *dev;
#define DRIVER_NAME "Template"
static struct file_operations fops = {};

int __init module_initialize(void)
{
	if (alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME)) {
		pr_err("Unable to allocate device number for device");
		return -EIO;
	}

	device_object = cdev_alloc();
	if (device_object == NULL) {
		pr_err("Unable to allocate character device object");
		goto free_device_number;
	}
	device_object->owner = THIS_MODULE;
	device_object->ops = &fops;
	if (cdev_add(device_object, dev_num, 1)) {
		pr_err("Unable to add character device object");
		goto free_cdev;
	}

	class = class_create(THIS_MODULE,
			     DRIVER_NAME);
	if (IS_ERR(class)) {
		pr_err("Unable to create class object for %s", DRIVER_NAME);
		goto free_cdev;
	}
	dev = device_create(class, NULL, dev_num, NULL, "%s", DRIVER_NAME);
	if (IS_ERR(dev)) {
		pr_err("Unable to create device object");
		goto free_class;
	}

	return 0;

 free_class:
	class_destroy(class);
 free_cdev:
	kobject_put(&device_object->kobj);
 free_device_number:
	unregister_chrdev_region(dev_num, 1);
	return -EIO;
}

void __exit module_cleanup(void)
{
	device_destroy(class, dev_num);
	class_destroy(class);
	cdev_del(device_object);
	unregister_chrdev_region(dev_num, 1);
}

module_init(module_initialize);
module_exit(module_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Homuth");
MODULE_DESCRIPTION("Driver Template");
