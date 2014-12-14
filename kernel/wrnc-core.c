/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
#include <linux/byteorder/generic.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include <linux/fmc.h>
#include <linux/fmc-sdb.h>

#include <hw/wrn_cpu_csr.h>
#include <hw/mqueue.h>

#include "wrnc.h"

static int wrnc_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0440);

	return 0;
}

static char *wrnc_devnode(struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "wr-node-core/%s", dev_name(dev));
}

static struct class wrnc_cdev_class = {
	.name		= "wr-node-core",
	.owner		= THIS_MODULE,
	.dev_uevent	= wrnc_dev_uevent,
	.devnode	= wrnc_devnode,
};

static dev_t basedev;
static struct cdev cdev_dev;
static struct cdev cdev_cpu;
static struct cdev cdev_hmq;

struct device *minors[WRNC_MAX_MINORS];


/**
 * It return the first available char device minor for a given type.
 */
static int wrnc_minor_get(struct device *dev, enum wrnc_dev_type type)
{
	int i, start, end;

	switch (type) {
	case WRNC_DEV:
		start = 0;
		end = start + WRNC_MAX_CARRIER;
		break;
	case WRNC_CPU:
		start = WRNC_MAX_CARRIER;
		end = start + WRNC_MAX_CPU_MINORS;
		break;
	case WRNC_HMQ:
		start = WRNC_MAX_CARRIER + WRNC_MAX_CPU_MINORS;
		end = start + WRNC_MAX_HMQ_MINORS;
		break;
	default:
		return -1;
	}

	for (i = start; i < end; ++i) {
		if (minors[i] == NULL) {
			minors[i] = dev;
			dev->devt = basedev + i;
			return 0;
		}
	}
	return -1;
}


/**
 * It releases the char device minor is use by a given device
 */
static void wrnc_minor_put(struct device *dev)
{
	int i;

	for (i = 0; i < WRNC_MAX_CPU_MINORS + WRNC_MAX_HMQ_MINORS; ++i) {
		if (minors[i] == dev) {
			minors[i] = NULL;
			break;
		}
	}
}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * SYSFS * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


/**
 * It returns the application ID
 */
static ssize_t wrnc_show_app_id(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);

	return sprintf(buf, "0x%x\n", wrnc->app_id);
}

/**
 * It returns the number of CPU in the FPGA
 */
static ssize_t wrnc_show_n_cpu(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);

	return sprintf(buf, "%d\n", wrnc->n_cpu);
}


/**
 * It returns the current enable status of the CPU
 */
static ssize_t wrnc_show_enable_mask(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);

	return sprintf(buf, "0x%04x\n", reg_val);
}

/**
 * It enable or disable the CPU
 */
static ssize_t wrnc_store_enable_mask(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	long val;

	if (kstrtol(buf, 16, &val))
		return -EINVAL;

	fmc_writel(fmc, val, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);

	return count;
}

/**
 * It returns the running status of the CPU
 */
static ssize_t wrnc_show_reset_mask(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_RESET);

	return sprintf(buf, "0x%04x\n", reg_val);
}

/**
 * It run or pause the CPU
 */
static ssize_t wrnc_store_reset_mask(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	long val;

	if (kstrtol(buf, 16, &val))
		return -EINVAL;

	fmc_writel(fmc, val, wrnc->base_csr + WRN_CPU_CSR_REG_RESET);

	return count;
}


static ssize_t wrnc_show_smem_op(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);

	return sprintf(buf, "%d", wrnc->mod);
}

static ssize_t wrnc_store_smem_op(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(dev);
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	if (val < WRNC_SMEM_DIRECT || val > WRNC_SMEM_ADD) {
		dev_err(&wrnc->dev, "Unsupported operation %ld\n", val);
		return -EINVAL;
	}

	wrnc->mod = val;

	return count;
}

DEVICE_ATTR(application_id, S_IRUGO, wrnc_show_app_id, NULL);
DEVICE_ATTR(n_cpu, S_IRUGO, wrnc_show_n_cpu, NULL);
DEVICE_ATTR(enable_mask, (S_IRUGO | S_IWUSR),
	    wrnc_show_enable_mask, wrnc_store_enable_mask);
DEVICE_ATTR(reset_mask, (S_IRUGO | S_IWUSR),
	    wrnc_show_reset_mask, wrnc_store_reset_mask);
DEVICE_ATTR(smem_operation, (S_IRUGO | S_IWUSR),
	    wrnc_show_smem_op, wrnc_store_smem_op);

static struct attribute *wrnc_dev_attr[] = {
	&dev_attr_application_id.attr,
	&dev_attr_n_cpu.attr,
	&dev_attr_enable_mask.attr,
	&dev_attr_reset_mask.attr,
	NULL,
};

static const struct attribute_group wrnc_dev_group = {
	.attrs = wrnc_dev_attr,
};

static const struct attribute_group *wrnc_dev_groups[] = {
	&wrnc_dev_group,
	NULL,
};





/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * DEV CHAR DEVICE * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Things to do after device release
 */
static void wrnc_dev_release(struct device *dev)
{

}

/**
 * ioctl command to read/write shared memory
 */
static long wrnc_ioctl_io(struct wrnc_dev *wrnc, void __user *uarg)
{
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	struct wrnc_smem_io io;
	uint32_t addr;
	int err;

	/* Copy the message from user space*/
	err = copy_from_user(&io, uarg, sizeof(struct wrnc_smem_io));
	if (err)
		return err;

	if (io.is_input) {
		/* read */
		addr = wrnc->base_smem + io.addr;
	} else {
		/* write */
		addr = wrnc->base_smem + (io.mod * WRNC_SMEM_MAX_SIZE)
			+ io.addr;
		fmc_writel(fmc, io.value, addr);
	}

	/* read value from SMEM */
	io.value = fmc_readl(fmc, addr);

	return copy_to_user(uarg, &io, sizeof(struct wrnc_smem_io));
}

static long wrnc_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct wrnc_dev *wrnc = f->private_data;
	void __user *uarg = (void __user *)arg;
	int err = 0;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != WRNC_IOCTL_MAGIC)
		return -ENOTTY;

	/* Validate user pointer */
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, uarg, _IOC_SIZE(cmd));
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, uarg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;

	/* Perform commands */
	switch (cmd) {
	case WRNC_IOCTL_SMEM_IO:
		err = wrnc_ioctl_io(wrnc, uarg);
		break;
	default:
		pr_warn("ual: invalid ioctl command %d\n", cmd);
		return -EINVAL;
	}

	return err;
}

/**
 * It writes on the shared memory
 */
static ssize_t wrnc_write(struct file *f, const char __user *buf,
			  size_t count, loff_t *offp)
{
	struct wrnc_dev *wrnc = f->private_data;
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t val, addr;
	int err, i;

	if (*offp + count >= WRNC_SMEM_MAX_SIZE)
		return -EINVAL;
	if (*offp % 4 || count % 4) {
		dev_warn(&wrnc->dev, "Only word size access allowed\n");
		return -EINVAL;
	}

	addr = wrnc->base_smem + (wrnc->mod * WRNC_SMEM_MAX_SIZE) + *offp;
	for (i = 0; i < count; i += 4) {
		err = copy_from_user(&val, buf + i, 4);
		if (err) {
			dev_err(&wrnc->dev,
				"Incomplete write on shared memory addr 0x%llx size %zu\n",
				*offp, count);
			return -EFAULT;
		}
		fmc_writel(fmc, val, addr + i);

	}

	*offp += count;

	return count;
}

/**
 * It reads from the shared memory
 */
static ssize_t wrnc_read(struct file *f, char __user *buf,
			 size_t count, loff_t *offp)
{
	struct wrnc_dev *wrnc = f->private_data;
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t val, addr;
	int err, i;

	if (*offp + count >= WRNC_SMEM_MAX_SIZE)
		return -EINVAL;
	if (*offp % 4 || count % 4) {
		dev_warn(&wrnc->dev, "Only word size access allowed\n");
		return -EINVAL;
	}

	addr = wrnc->base_smem + *offp;
	for (i = 0; i < count; i += 4) {
		val = fmc_readl(fmc,  addr + i);
		err = copy_to_user(buf + i, &val, 4);
		if (err)
			return -EFAULT;
	}

	*offp += count;

	return count;
}


/**
 * Open the char device on the top of the hierarchy
 */
static int wrnc_dev_simple_open(struct inode *inode, struct file *file)
{
	int m = iminor(inode);

	file->private_data = to_wrnc_dev(minors[m]);

	return 0;
}

static const struct file_operations wrnc_dev_fops = {
	.owner = THIS_MODULE,
	.open  = wrnc_dev_simple_open,
	.read = wrnc_read,
	.write = wrnc_write,
	.llseek = generic_file_llseek,
	.unlocked_ioctl = wrnc_ioctl,
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * DRIVER (un)LOADING  * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Things to do after CPU device release
 */
static void wrnc_cpu_release(struct device *dev)
{
	/*wrnc_minor_put(dev);*/
}

/**
 * Things to do after HMQ device release
 */
static void wrnc_hmq_release(struct device *dev)
{
	/*wrnc_minor_put(dev);*/
}


/**
 * It initializes and registers a HMQ device
 */
static int wrnc_probe_hmq(struct wrnc_dev *wrnc, unsigned int slot,
			  unsigned int is_input)
{
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	struct wrnc_hmq *hmq;
	int err;

	hmq = is_input ? &wrnc->hmq_in[slot] : &wrnc->hmq_out[slot];

	hmq->index = slot;

	err = wrnc_minor_get(&hmq->dev, WRNC_HMQ);
	if (err)
		return err;

	err = dev_set_name(&hmq->dev, "wrnc-%04x-hmq-%c-%02d",
			   fmc->device_id, is_input ? 'i':'o', hmq->index);
	if (err)
		return err;

	init_waitqueue_head(&hmq->q_msg);
	hmq->dev.class = &wrnc_cdev_class;
	hmq->dev.parent = &wrnc->dev;
	hmq->dev.groups = wrnc_hmq_groups;
	hmq->dev.release = wrnc_hmq_release;
	err = device_register(&hmq->dev);
	if (err)
		return err;

	INIT_LIST_HEAD(&hmq->list_msg);
	if (is_input) { /* CPU input */
		hmq->flags |= WRNC_FLAG_HMQ_DIR;
		hmq->base_sr = wrnc->base_hmq +	MQUEUE_BASE_IN(slot);
	} else { /* CPU output */
		hmq->base_sr = wrnc->base_hmq +	MQUEUE_BASE_OUT(slot);
	}
	spin_lock_init(&hmq->lock);
	/* Flush the content of the slot */
	fmc_writel(fmc, MQUEUE_CMD_PURGE,
		   hmq->base_sr + MQUEUE_SLOT_COMMAND);

	hmq->count = 0;

	return 0;
}

/**
 * It initialize the WRNC device (device, CPUs, HMQs)
 */
int wrnc_probe(struct fmc_device *fmc)
{
	struct wrnc_dev *wrnc;
	int err, i;
	uint32_t tmp;

	/* Create a WRNC instance */
	wrnc = devm_kzalloc(&fmc->dev, sizeof(struct wrnc_dev), GFP_KERNEL);
	if (!wrnc)
		return -ENOMEM;
	fmc_set_drvdata(fmc, wrnc);

	err = fmc_scan_sdb_tree(fmc, 0x0);
	if (err < 0 && err != -EBUSY) {
		dev_err(fmc->hwdev, "SDB is missing\n");
		return err;
	}
	wrnc->base_core = fmc_find_sdb_device(fmc->sdb, 0xce42, 0x90de, NULL);
	/* FIXME use SDB - <base> + <CSR offset> */
	wrnc->base_csr  = wrnc->base_core + 0x10000;
	wrnc->base_smem = wrnc->base_core + 0x20000;
	wrnc->base_hmq  = wrnc->base_core + BASE_HMQ;
	wrnc->base_gcr  = wrnc->base_hmq + MQUEUE_BASE_GCR;

	/* Register the device */
	err = dev_set_name(&wrnc->dev, "wrnc-%04x", fmc->device_id);
	if (err)
		return err;
	err = wrnc_minor_get(&wrnc->dev, WRNC_DEV);
	if (err)
		return err;
	wrnc->dev.class = &wrnc_cdev_class;
	wrnc->dev.parent = &fmc->dev;
	wrnc->dev.groups = wrnc_dev_groups;
	wrnc->dev.release = wrnc_dev_release;
	err = device_register(&wrnc->dev);
	if (err)
		return err;


	/* Get the Application ID */
	wrnc->app_id = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_APP_ID);
	dev_info(&fmc->dev, "Application ID: 0x%08x\n", wrnc->app_id);

	/* Get and check the number of COREs */
	wrnc->n_cpu = fmc_readl(fmc,
				wrnc->base_csr + WRN_CPU_CSR_REG_CORE_COUNT);
	if (wrnc->n_cpu < 1 || wrnc->n_cpu > WRNC_MAX_CPU) {
		dev_err(&fmc->dev, "invalid number of CPU (%d)\n", wrnc->n_cpu);
		err = -EINVAL;
		goto out_n_cpu;
	}
	dev_info(&fmc->dev, "Detected %d CPUs\n", wrnc->n_cpu);

	/* Pause all CPUs */
	wrnc_cpu_enable_set(wrnc, (1 << wrnc->n_cpu) - 1);
	/* Reset all CPUs */
	wrnc_cpu_reset_set(wrnc, (1 << wrnc->n_cpu) - 1);

	/* Configure CPUs */
	for (i = 0; i < wrnc->n_cpu; ++i) {
		wrnc->cpu[i].index = i;

		err = wrnc_minor_get(&wrnc->cpu[i].dev, WRNC_CPU);
		if (err)
			goto out_cpu;
		err = dev_set_name(&wrnc->cpu[i].dev, "wrnc-%04x-cpu-%02d",
				   fmc->device_id, wrnc->cpu[i].index);
		if (err)
			goto out_cpu;
		wrnc->cpu[i].dev.class = &wrnc_cdev_class;
		wrnc->cpu[i].dev.parent = &wrnc->dev;
		wrnc->cpu[i].dev.groups = wrnc_cpu_groups;
		wrnc->cpu[i].dev.release = wrnc_cpu_release;
		err = device_register(&wrnc->cpu[i].dev);
		if (err)
			goto out_cpu;
	}

	/* Get and check the number of HMQ slots */
	tmp = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_SLOT_COUNT);
	wrnc->n_hmq_in = tmp & MQUEUE_GCR_SLOT_COUNT_N_IN_MASK;
	wrnc->n_hmq_out = (tmp & MQUEUE_GCR_SLOT_COUNT_N_OUT_MASK) >>
		MQUEUE_GCR_SLOT_COUNT_N_OUT_SHIFT;
	if (wrnc->n_hmq_in + wrnc->n_hmq_out >= WRNC_MAX_HMQ_SLOT) {
		dev_err(&fmc->dev, "wrnc: invalid number of HMQ slots (in %d out %d)\n",
			 wrnc->n_hmq_in, wrnc->n_hmq_out);
		err = -EINVAL;
		goto out_n_slot;
	}
	dev_info(&fmc->dev, "Detected slots: %d input, %d output\n",
		wrnc->n_hmq_in, wrnc->n_hmq_out);

	/* Configure slots */
	for (i = 0; i < wrnc->n_hmq_in; ++i) {
		err = wrnc_probe_hmq(wrnc, i, 1);
		if (err)
			goto out_hmq_in;
	}
	for (i = 0; i <  wrnc->n_hmq_out; ++i) {
		err = wrnc_probe_hmq(wrnc, i, 0);
		if (err)
			goto out_hmq_out;
	}

	/*
	 * Great everything is configured properly, we can enable the interrupts
	 * now and start working.
	 */
	fmc->irq = wrnc->base_core;
	err = fmc->op->irq_request(fmc, wrnc_irq_handler,
				   (char *)dev_name(&wrnc->dev),
				   0 /*VIC is used */);
	if (err) {
		dev_err(&wrnc->dev,
			"Cannot request IRQ 0x%x - we'll not receive/send messages\n",
			fmc->irq);
	}

	/*
	 * Don't raise interrupts on output for the time being. we are
	 * going to use only synchronous messages
	 */
	if (0)
		wrnc->irq_mask = (((1 << wrnc->n_hmq_in) - 1)
				  << MQUEUE_GCR_IRQ_MASK_IN_SHIFT);
	else
		wrnc->irq_mask = 0;
	wrnc->irq_mask |= (1 << wrnc->n_hmq_out) - 1;
	fmc_writel(fmc, wrnc->irq_mask, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
	tmp = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);

	return 0;

out_hmq_out:
	while (--i)
		device_unregister(&wrnc->hmq_out[i].dev);
	i = wrnc->n_hmq_out;
out_hmq_in:
	while (--i)
		device_unregister(&wrnc->hmq_in[i].dev);
out_n_slot:
	i = wrnc->n_cpu;
out_cpu:
	while (--i)
		device_unregister(&wrnc->cpu[i].dev);
out_n_cpu:
	device_unregister(&wrnc->dev);
	return err;
}

/**
 * It remove the WRNC device (device, CPUs, HMQs) and free irq handler
 */
int wrnc_remove(struct fmc_device *fmc)
{
	struct wrnc_dev *wrnc = fmc_get_drvdata(fmc);
	int i;

	fmc_writel(fmc, 0x0, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
	fmc->irq = 0xC0000;
	fmc->op->irq_free(fmc);

	for (i = 0; i < wrnc->n_cpu; ++i)
		device_unregister(&wrnc->cpu[i].dev);

	for (i = 0; i < wrnc->n_hmq_in; ++i)
		device_unregister(&wrnc->hmq_in[i].dev);

	for (i = 0; i < wrnc->n_hmq_out; ++i)
		device_unregister(&wrnc->hmq_out[i].dev);

	/* FIXME cannot explain why, but without sleep the _kernel_ crash */
	msleep(50);
	device_unregister(&wrnc->dev);

	return 0;
}


/**
 * List of device to match.
 *
 * @FIXME for the time being we match the TDC mezzanine because we do not have
 *        the capability to match single FPGA components
 */
static struct fmc_fru_id wrnc_fru_id[] = {
	{
		.product_name = "wr-node-core"
	},
};

static struct fmc_driver wrnc_dev_drv = {
	.version = FMC_VERSION,
	.driver.name = KBUILD_MODNAME,
	.probe = wrnc_probe,
	.remove = wrnc_remove,
	.id_table = {
		.fru_id = wrnc_fru_id,
		.fru_id_nr = ARRAY_SIZE(wrnc_fru_id),
	},
};


/**
 * Allocate resources for the driver. Char devices and FMC driver
 */
static int wrnc_init(void)
{
	int err, i;

	for (i = 0; i < WRNC_MAX_CPU_MINORS; ++i)
		minors[i] = NULL;

	err = class_register(&wrnc_cdev_class);
	if (err) {
		pr_err("%s: unable to register class\n", __func__);
		return err;
	}

	/* Allocate a char device region for devices, CPUs and slots */
	err = alloc_chrdev_region(&basedev, 0, WRNC_MAX_MINORS, "wrnc");
	if (err) {
		pr_err("%s: unable to allocate region for %i minors\n",
		       __func__, WRNC_MAX_CPU_MINORS);
		goto out_all;
	}

	/* Register the device char-device */
	cdev_init(&cdev_dev, &wrnc_dev_fops);
	cdev_dev.owner = THIS_MODULE;
	err = cdev_add(&cdev_dev, basedev, WRNC_MAX_CARRIER);
	if (err)
		goto out_cdev_dev;
	/* Register the cpu char-device */
	cdev_init(&cdev_cpu, &wrnc_cpu_fops);
	cdev_cpu.owner = THIS_MODULE;
	err = cdev_add(&cdev_cpu, basedev + WRNC_MAX_CARRIER,
		       WRNC_MAX_CPU_MINORS);
	if (err)
		goto out_cdev_cpu;
	/* Register the hmq char-device */
	cdev_init(&cdev_hmq, &wrnc_hmq_fops);
	cdev_cpu.owner = THIS_MODULE;
	err = cdev_add(&cdev_hmq,
		       basedev + WRNC_MAX_CARRIER + WRNC_MAX_CPU_MINORS,
		       WRNC_MAX_HMQ_MINORS);
	if (err)
		goto out_cdev_hmq;

	/* Register the FMC driver */
	err = fmc_driver_register(&wrnc_dev_drv);
	if (err)
		goto out_reg;

	return 0;

out_reg:
	cdev_del(&cdev_hmq);
out_cdev_hmq:
	cdev_del(&cdev_cpu);
out_cdev_cpu:
	cdev_del(&cdev_dev);
out_cdev_dev:
	unregister_chrdev_region(basedev, WRNC_MAX_MINORS);
out_all:
	class_unregister(&wrnc_cdev_class);
	return err;
}


/**
 * Undo all the resource allocations
 */
static void wrnc_exit(void)
{
	fmc_driver_unregister(&wrnc_dev_drv);
	cdev_del(&cdev_cpu);
	unregister_chrdev_region(basedev,
				 WRNC_MAX_CPU_MINORS + WRNC_MAX_HMQ_MINORS);
	class_unregister(&wrnc_cdev_class);
}

module_init(wrnc_init);
module_exit(wrnc_exit);

MODULE_AUTHOR("Federico Vaga <federico.vaga@cern.ch>");
MODULE_DESCRIPTION("White Rabbit Node Core Linux Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(GIT_VERSION);

CERN_SUPER_MODULE;
