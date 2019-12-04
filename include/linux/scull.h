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

#define SCULL_IOC_MAGIC 0xe4

#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)

/* Set through a pointer */
#define SCULL_IOCSQUANTUM   _IOW(SCULL_IOC_MAGIC, 1, int)
#define SCULL_IOCSQSET      _IOW(SCULL_IOC_MAGIC, 2, int)
/* Tell with an argument value */
#define SCULL_IOCTQUANTUM   _IO(SCULL_IOC_MAGIC, 3)
#define SCULL_IOCTQSET      _IO(SCULL_IOC_MAGIC, 4)
/* Get reply by setting throuh a pointer */
#define SCULL_IOCGQUANTUM   _IOR(SCULL_IOC_MAGIC, 5, int)
#define SCULL_IOCGQSET      _IOR(SCULL_IOC_MAGIC, 6, int)
/* Query response is on the return value */
#define SCULL_IOCQQUANTUM   _IO(SCULL_IOC_MAGIC, 7)
#define SCULL_IOCQQSET      _IO(SCULL_IOC_MAGIC, 8)
/* eXchange switches G and S atomically */
#define SCULL_IOCXQUANTUM   _IOWR(SCULL_IOC_MAGIC, 9, int)
#define SCULL_IOCXQSET      _IOWR(SCULL_IOC_MAGIC, 10, int)
/* sHift switches T and Q atomically */
#define SCULL_IOCHQUANTUM   _IO(SCULL_IOC_MAGIC, 11)
#define SCULL_IOCHQSET      _IO(SCULL_IOC_MAGIC, 12)

#define SCULL_IOC_MAXNR 14
#endif
