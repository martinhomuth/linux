// SPDX-License-Identifier: GPL-2.0-only
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/scull.h>
#include <linux/slab.h>

#define NUM_SCULL_DEVICES 4

static dev_t dev_num;
static struct class *scull_class;
#define DRIVER_NAME "scull"
static const struct file_operations scull_fops = {};
static int scull_major;
static int nr_scull_devices = NUM_SCULL_DEVICES;
module_param(nr_scull_devices, int, 0444);

static struct scull_dev *scull_devices;

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
}

static int __init module_initialize(void)
{
	int i = 0;

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
