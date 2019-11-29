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
#include <linux/fs.h>    /* register_chrdev_region */
#include <linux/cdev.h>
#include <linux/device.h>

static dev_t dev_num;
static struct cdev *device_object;
static struct class *class;
#define DRIVER_NAME "TestDriver"
static struct file_operations fops = {
				      /* here the addresses of the
				       * actual device functions are
				       * stored
				       */
};

int __init module_initialize(void)
{
	/* Obtain a valid device number from the kernel */
	if (alloc_chrdev_region(
				&dev_num,    /* the dev_t structure to
						fill */
				0,           /* baseminor */
				1,           /* count */
				DRIVER_NAME  /* name */
				)) {
		pr_err("Unable to allocate device number for device");
		return -EIO;
	}
	/* allocate the character device structure */
	device_object = cdev_alloc();
	if (device_object == NULL)
		goto free_device_number;
	/* and fill the relevant information */
	device_object->owner = THIS_MODULE;
	device_object->ops = &fops;
	/* register driver ith the kernel */
	if (cdev_add(
		     device_object,       /* the cdev driver to add */
		     dev_num,             /* device number of the
					     driver */
		     1                    /* count */
		     ))
		goto free_cdev;

	/* add sysfs entry for udev */
	class = class_create(THIS_MODULE,
			     DRIVER_NAME);
	if (IS_ERR(class))
		goto free_cdev;
	device_create(
		      class,          /* class of the device */
		      NULL,           /* parent, where to put it in sysfs */
		      dev_num,        /* device number (major/minor) */
		      NULL,           /* associated device */
		      "%s",           /* fmt of the name of the entry */
		      DRIVER_NAME
		      );

	return 0;

 free_cdev:
	kobject_put(&device_object->kobj);
 free_device_number:
	unregister_chrdev_region(dev_num, 1);
	return -EIO;
}

void __exit module_cleanup(void)
{
	/* delete sysfs entry first */
	device_destroy(
		       class,         /* class object */
		       dev_num        /* device number */
		       );
	class_destroy(class);

	/* unregistering the driver */
	cdev_del(device_object);
	unregister_chrdev_region(dev_num, 1);
}

module_init(module_initialize);
module_exit(module_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Homuth");
MODULE_DESCRIPTION("More sophisticated driver module");
