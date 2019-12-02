// SPDX-License-Identifier: GPL-2.0-only
#define DEBUG 1
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/scull.h>
#include <linux/slab.h>

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
			pr_debug("Freeing qset 0x%p\n", dptr->data);
			for (i = 0; i < qset; i++) {
				if (dptr->data[i])
					pr_debug("freeing quantum %d\n", i);
				else
					pr_debug("unset quantum %d\n", i);
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

static const struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
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

	for (i = 0; i < nr_scull_devices; i++)
		scull_setup_cdev(&scull_devices[i], i);

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
