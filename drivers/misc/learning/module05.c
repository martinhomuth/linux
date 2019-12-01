#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

static dev_t dev_num;
static struct cdev *device_object;
static struct class *class;
static struct device *module05_dev;
#define DRIVER_NAME "module05"
#define BUFSIZE 64

static char kbuf[BUFSIZE];

static int device_open(struct inode *inode, struct file *filep)
{
	dev_info(module05_dev, "%s opened by %d with mode %x ", DRIVER_NAME,
		 inode->i_uid.val, filep->f_flags);

	/* perform some initialization */

	return 0;
}

static int device_close(struct inode *inode, struct file *filep)
{
	dev_info(module05_dev, "%s closed by %d", DRIVER_NAME, inode->i_uid.val);
	/* perform some cleanup */

	return 0;
}

static ssize_t device_read (struct file *filep, char __user *buf,
			    size_t count, loff_t *ppos)
{
	unsigned not_copied, to_copy;
	dev_info(module05_dev, "uid %d reads %ld bytes",
		 filep->f_inode->i_uid.val,
		 count);

	to_copy = min(count, (size_t) BUFSIZE);
	// void __user *to, const void *from, unsigned long n)
	not_copied = copy_to_user(buf, kbuf, to_copy);
	*ppos += to_copy - not_copied;
	return to_copy - not_copied;
}

static ssize_t device_write(struct file *filep, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	unsigned not_copied, to_copy;
	to_copy = min(count, (size_t) BUFSIZE);
	not_copied = copy_from_user(kbuf, buf, to_copy);
	*ppos += to_copy - not_copied;
	return to_copy - not_copied;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_close,

	/* ssize_t (*read) (struct file *, char __user *, size_t,
	   loff_t *); */
	.read = device_read,
	/* ssize_t (*write) (struct file *, const char __user *,
	   size_t, loff_t *); */
	.write = device_write,
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
	module05_dev = device_create(class, NULL, dev_num, NULL, "%s", DRIVER_NAME);
	if (IS_ERR(module05_dev)) {
		pr_err("module05: device_create failed\n");
		goto free_class;
	}

	memset(kbuf, 0, BUFSIZE);

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
MODULE_DESCRIPTION("Module05 - reading and writing");
