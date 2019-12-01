#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/spinlock.h>

static dev_t dev_num;
static struct cdev *device_object;
static struct class *class;
static struct device *module07_dev;
#define DRIVER_NAME "module07"
#define BUFSIZE 1024

static char kbuf[1024];
atomic_t bufsize;
spinlock_t driver_lock;
DECLARE_WAIT_QUEUE_HEAD(read_queue);

static int dev_open(struct inode *inode, struct file *filep)
{
	pr_info("Opened by %d with mode %x ",
		 inode->i_uid.val, filep->f_flags);
	return 0;
}

static int dev_close(struct inode *inode, struct file *filep)
{
	pr_info("Closed by %d", inode->i_uid.val);
	return 0;
}

static __poll_t dev_poll(struct file *filep, struct poll_table_struct *pt)
{
	__poll_t mask = 0;
	spin_lock(&driver_lock);

	poll_wait(filep, &read_queue, pt);

	if (atomic_read(&bufsize) != 0) {
		mask |= POLLIN | POLLRDNORM;
	}

	spin_unlock(&driver_lock);
	return mask;
}

static ssize_t dev_read(struct file *filep, char __user *buf,
			size_t count, loff_t* ppos)
{
	unsigned not_copied, to_copy;
	to_copy = min(count, atomic_read(&bufsize));
	not_copied = copy_to_user(buf, kbuf, to_copy);
	*ppos += to_copy - not_copied;
	atomic_set(&bufsize, atomic_read(&bufsize) - to_copy - not_copied);
	return to_copy - not_copied;
}

static ssize_t dev_write(struct file *filep, const char __user *buf,
			 size_t count, loff_t* ppos)
{
	unsigned not_copied, to_copy;
	to_copy = min(count, (size_t)BUFSIZE);
	not_copied = copy_from_user(kbuf, buf, to_copy);
	*ppos += to_copy - not_copied;
	atomic_set(&bufsize, atomic_read(&bufsize) + to_copy - not_copied);
	return to_copy - not_copied;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_close,
	.write = dev_write,
	.read = dev_read,
	/* __poll_t (*poll) (struct file *,
	 *                   struct poll_table_struct *);
	 */
	.poll = dev_poll,
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
	module07_dev = device_create(class, NULL, dev_num, NULL, "%s", DRIVER_NAME);
	if (IS_ERR(module07_dev))
		goto free_class;

	memset(kbuf, 0, BUFSIZE);
	atomic_set(&bufsize, 0);
	spin_lock_init(&driver_lock);

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
MODULE_DESCRIPTION("Module07 - poll");
