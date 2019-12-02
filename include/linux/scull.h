#ifndef __SCULL_H
#define __SCULL_H

#include <linux/cdev.h>
#include <linux/semaphore.h>

struct scull_qset {
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;   /* Pointer to first quantum set */
	int quantum;               /* the current quantum size */
	int qset;                  /* the current array size */
	unsigned long size;        /* amount of data stored here */
	unsigned int access_key;   /* used by sculluid scullpriv */
	struct rw_semaphore rwsem; /* mutual exclusion semaphore */
	struct cdev cdev;          /* char device structure */
	struct device *device;     /* device structure */
};

#endif
