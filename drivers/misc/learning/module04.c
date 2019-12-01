#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

static dev_t dev_num;
static struct cdev *device_object;
static struct class *class;
static struct device *module04_dev;
#define DRIVER_NAME "module04"

static int device_open(struct inode *inode,  /* user space process
						information */
		       struct file *filep    /* device driver information */
		       )
{
	dev_info(module04_dev, "%s opened by %d with mode %x ", DRIVER_NAME,
		 inode->i_uid.val, filep->f_flags);

	/* check for permissions (assume only RO access allowed) */
	if ((filep->f_flags & O_ACCMODE) != O_RDONLY)
		return -EPERM;

	/* perform some initialization */

	return 0;
}

static int device_close(struct inode *inode, struct file *filep)
{
	dev_info(module04_dev, "%s closed by %d", DRIVER_NAME, inode->i_uid.val);
	/* perform some cleanup */

	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
        /* int (*open) (struct inode *, struct file *); */
	.open = device_open,
	/* int (*release) (struct inode *, struct file *); */
	.release = device_close,
};

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
	module04_dev = device_create(class, NULL, dev_num, NULL, "%s", DRIVER_NAME);
	if (IS_ERR(module04_dev)) {
		pr_err("module04: device_create failed\n");
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
MODULE_DESCRIPTION("Module04 - open and close");
