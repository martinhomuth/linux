// SPDX-License-Identifier: GPL-2.0-only
#define DEBUG 1
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/scull.h>
#include <linux/slab.h>
#include <linux/rwsem.h>
#include <linux/uaccess.h>

#define NUM_SCULL_DEVICES 4

#define SCULL_QUANTUM_SIZE 4000

#define SCULL_QSET_SIZE 4000

static dev_t dev_num;
static struct class *scull_class;
#define DRIVER_NAME "scull"
static int scull_major;
static int nr_scull_devices = NUM_SCULL_DEVICES;
module_param(nr_scull_devices, int, 0444);

static int scull_qset_size = SCULL_QSET_SIZE;
module_param(scull_qset_size, int, 0444);
static int scull_quantum_size = SCULL_QUANTUM_SIZE;
module_param(scull_quantum_size, int, 0444);

static struct scull_dev *scull_devices;

/*
 * Freeing the whole scull data structure
 */
static int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next, *dptr;
	int qset = dev->qset;
	int i;

	pr_debug("%s\n", __func__);
	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr->data) {
			pr_debug("freeing qset 0x%p\n", dptr->data);
			for (i = 0; i < qset; i++) {
				if (dptr->data[i])
					pr_debug("freeing quantum %d\n", i);
				kfree(dptr->data[i]);
			}
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->qset = scull_qset_size;
	dev->quantum = scull_quantum_size;
	dev->data = NULL;
	return 0;
}

static int scull_open(struct inode *inode, struct file *filep)
{
	struct scull_dev *dev;

	pr_debug("%s\n", __func__);
	/* 1. check for device specific errors */

	/* 2. initialize the device if opened for the first time */

	/* 3. update the f_op pointer if necessary */

	/* 4. allocate and fill any data structure to be put in
	 *   filep->private_data
	 */

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filep->private_data = dev; /* for other methods */

	if ((filep->f_flags & O_ACCMODE) == O_WRONLY)
		scull_trim(dev);

	return 0;
}

static int scull_release(struct inode *inode, struct file *filep)
{
	pr_debug("%s\n", __func__);
	/* 1. deallocate anything that open allocated in
	 * filep->private_data
	 */

	/* 2. shut down the device on last close */

	return 0;
}

static struct scull_qset *scull_follow(struct scull_dev *dev, int item)
{
	struct scull_qset *qs = dev->data;

	if (!qs) {
		qs = dev->data = kzalloc(sizeof(struct scull_qset), GFP_KERNEL);
		if (!qs)
			return NULL;
		pr_debug("allocating qset 0x%p\n", qs);
	}

	while (item--) {
		if (!qs->next) {
			qs->next = kzalloc(sizeof(struct scull_qset),
					   GFP_KERNEL);
			if (!qs->next)
				return NULL;
			pr_debug("allocating qset 0x%p\n", qs);
		}
		qs = qs->next;
		continue;
	}
	return qs;
}

static ssize_t scull_read(struct file *filep, char __user *buf,
			  size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filep->private_data;
	struct scull_qset *dptr; /* the first listitem */
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset; /* bytes in listitem */
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

	pr_debug("%s\n", __func__);
	down_read(&dev->rwsem);

	if (*f_pos > dev->size)
		goto out;

	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/* find listitem, qset index, and offset in the quantum */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;
	/* follow the list up to the right position */
	dptr = scull_follow(dev, item);

	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		goto out; /* don't fill holes */

	/* read only up to the end of the quantum */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;

 out:
	up_read(&dev->rwsem);
	return retval;
}

static ssize_t scull_write(struct file *filep, const char __user *buf,
			   size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filep->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM;

	pr_debug("%s\n", __func__);

	down_write(&dev->rwsem);

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	dptr = scull_follow(dev, item);
	if (dptr == NULL)
		goto out;
	if (!dptr->data) {
		dptr->data = kcalloc(qset, sizeof(char *), GFP_KERNEL);
		pr_debug("allocating qset 0x%p\n", dptr->data);
		if (!dptr->data)
			goto out;
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		pr_debug("allocating quantum %d\n", s_pos);
		if (!dptr->data[s_pos])
			goto out;
	}
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos] + q_pos,
			   buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

	if (dev->size < *f_pos)
		dev->size = *f_pos;

 out:
	up_write(&dev->rwsem);
	return count;
}

static const struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
	.read = scull_read,
	.write = scull_write,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err;

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;
	err = cdev_add(&dev->cdev, MKDEV(scull_major, index), 1);
	if (err)
		pr_err("Error %d adding scull devices", err);
}

static void scull_cleanup_cdev(struct scull_dev *dev, int index)
{
	cdev_del(&dev->cdev);
	scull_trim(dev);
}

static int __init module_initialize(void)
{
	int i = 0;

	pr_debug("%s\n", __func__);

	if (alloc_chrdev_region(&dev_num, 0, nr_scull_devices, DRIVER_NAME)) {
		pr_err("Unable to allocate device number for device\n");
		return -EIO;
	}
	scull_major = MAJOR(dev_num);

	scull_devices = kcalloc(nr_scull_devices,
				sizeof(struct scull_dev),
				GFP_KERNEL);

	scull_class = class_create(THIS_MODULE,
				   DRIVER_NAME);
	if (IS_ERR(scull_class)) {
		pr_err("Unable to create class object for %s", DRIVER_NAME);
		goto free_cdev;
	}
	for (i = 0; i < nr_scull_devices; i++) {
		(&scull_devices[i])->device = device_create(scull_class, NULL,
							  MKDEV(scull_major, i),
							  NULL, "%s%d",
							  DRIVER_NAME, i);
		if (IS_ERR((&scull_devices[i])->device)) {
			pr_err("Unable to create scull device %d", i);
			goto free_class;
		}
	}

	for (i = 0; i < nr_scull_devices; i++) {
		scull_setup_cdev(&scull_devices[i], i);
		init_rwsem(&scull_devices[i].rwsem);
	}

	return 0;

 free_class:
	class_destroy(scull_class);
 free_cdev:
	for (i = 0; i < nr_scull_devices; i++)
		scull_cleanup_cdev(&scull_devices[i], i);
 free_device_number:
	unregister_chrdev_region(dev_num, 4);
	return -EIO;

}

static void __exit module_cleanup(void)
{
	int i;

	pr_debug("%s\n", __func__);
	for (i = 0; i < nr_scull_devices; i++)
		device_destroy(scull_class, MKDEV(scull_major, i));
	class_destroy(scull_class);
	if (scull_devices)
		for (i = 0; i < nr_scull_devices; i++)
			scull_cleanup_cdev(&scull_devices[i], i);
	unregister_chrdev_region(dev_num, 4);
}

module_init(module_initialize);
module_exit(module_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Martin Homuth");
MODULE_DESCRIPTION("Linux Device Drivers Scull Module");
