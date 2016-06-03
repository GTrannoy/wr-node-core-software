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
#include <linux/debugfs.h>
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

#include <hw/mockturtle_cpu_csr.h>
#include <hw/mockturtle_queue.h>

#include "mockturtle-drv.h"

static int trtl_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0440);

	return 0;
}

static char *trtl_devnode(struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "mockturtle/%s", dev_name(dev));
}

static struct class trtl_cdev_class = {
	.name		= "mockturtle",
	.owner		= THIS_MODULE,
	.dev_uevent	= trtl_dev_uevent,
	.devnode	= trtl_devnode,
};

static dev_t basedev;
static struct cdev cdev_dev;
static struct cdev cdev_cpu;
static struct cdev cdev_hmq;

struct device *minors[TRTL_MAX_MINORS];


/**
 * It return the first available char device minor for a given type.
 */
static int trtl_minor_get(struct device *dev, enum trtl_dev_type type)
{
	int i, start, end;

	switch (type) {
	case TRTL_DEV:
		start = 0;
		end = start + TRTL_MAX_CARRIER;
		break;
	case TRTL_CPU:
		start = TRTL_MAX_CARRIER;
		end = start + TRTL_MAX_CPU_MINORS;
		break;
	case TRTL_HMQ:
		start = TRTL_MAX_CARRIER + TRTL_MAX_CPU_MINORS;
		end = start + TRTL_MAX_HMQ_MINORS;
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
static void trtl_minor_put(struct device *dev)
{
	int i;

	for (i = 0; i < TRTL_MAX_CPU_MINORS + TRTL_MAX_HMQ_MINORS; ++i) {
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
static ssize_t trtl_show_app_id(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);

	return sprintf(buf, "0x%x\n", trtl->app_id);
}

/**
 * It returns the number of CPU in the FPGA
 */
static ssize_t trtl_show_n_cpu(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);

	return sprintf(buf, "%d\n", trtl->n_cpu);
}


/**
 * It returns the current enable status of the CPU
 */
static ssize_t trtl_show_enable_mask(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_ENABLE);

	return sprintf(buf, "0x%04x\n", reg_val);
}

/**
 * It enable or disable the CPU
 */
static ssize_t trtl_store_enable_mask(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	long val;

	if (kstrtol(buf, 16, &val))
		return -EINVAL;

	fmc_writel(fmc, val, trtl->base_csr + WRN_CPU_CSR_REG_ENABLE);

	return count;
}

/**
 * It returns the running status of the CPU
 */
static ssize_t trtl_show_reset_mask(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_RESET);

	return sprintf(buf, "0x%04x\n", reg_val);
}

/**
 * It run or pause the CPU
 */
static ssize_t trtl_store_reset_mask(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	long val;

	if (kstrtol(buf, 16, &val))
		return -EINVAL;

	fmc_writel(fmc, val, trtl->base_csr + WRN_CPU_CSR_REG_RESET);

	return count;
}


static ssize_t trtl_show_smem_op(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);

	return sprintf(buf, "%d", trtl->mod);
}

static ssize_t trtl_store_smem_op(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct trtl_dev *trtl = to_trtl_dev(dev);
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	if (val < TRTL_SMEM_DIRECT || val > TRTL_SMEM_ADD) {
		dev_err(&trtl->dev, "Unsupported operation %ld\n", val);
		return -EINVAL;
	}

	trtl->mod = val;

	return count;
}

DEVICE_ATTR(application_id, S_IRUGO, trtl_show_app_id, NULL);
DEVICE_ATTR(n_cpu, S_IRUGO, trtl_show_n_cpu, NULL);
DEVICE_ATTR(enable_mask, (S_IRUGO | S_IWUSR),
	    trtl_show_enable_mask, trtl_store_enable_mask);
DEVICE_ATTR(reset_mask, (S_IRUGO | S_IWUSR),
	    trtl_show_reset_mask, trtl_store_reset_mask);
DEVICE_ATTR(smem_operation, (S_IRUGO | S_IWUSR),
	    trtl_show_smem_op, trtl_store_smem_op);

static struct attribute *trtl_dev_attr[] = {
	&dev_attr_application_id.attr,
	&dev_attr_n_cpu.attr,
	&dev_attr_enable_mask.attr,
	&dev_attr_reset_mask.attr,
	NULL,
};

static const struct attribute_group trtl_dev_group = {
	.attrs = trtl_dev_attr,
};

static const struct attribute_group *trtl_dev_groups[] = {
	&trtl_dev_group,
	NULL,
};





/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * DEV CHAR DEVICE * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Things to do after device release
 */
static void trtl_dev_release(struct device *dev)
{

}

/**
 * ioctl command to read/write shared memory
 */
static long trtl_ioctl_io(struct trtl_dev *trtl, void __user *uarg)
{
	struct fmc_device *fmc = to_fmc_dev(trtl);
	struct trtl_smem_io io;
	uint32_t addr;
	int err;

	/* Copy the message from user space*/
	err = copy_from_user(&io, uarg, sizeof(struct trtl_smem_io));
	if (err)
		return err;

	if (io.is_input) {
		/* read */
		addr = trtl->base_smem + io.addr;
	} else {
		fmc_writel(fmc, io.mod, trtl->base_csr + WRN_CPU_CSR_REG_SMEM_OP);
		/* write */
		addr = trtl->base_smem + io.addr;
		fmc_writel(fmc, io.value, addr);
	}

	/* read value from SMEM */
	io.value = fmc_readl(fmc, addr);

	return copy_to_user(uarg, &io, sizeof(struct trtl_smem_io));
}

static long trtl_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct trtl_dev *trtl = f->private_data;
	void __user *uarg = (void __user *)arg;
	int err = 0;

	/* Check type and command number */
	if (_IOC_TYPE(cmd) != TRTL_IOCTL_MAGIC)
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
	case TRTL_IOCTL_SMEM_IO:
		err = trtl_ioctl_io(trtl, uarg);
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
static ssize_t trtl_write(struct file *f, const char __user *buf,
			  size_t count, loff_t *offp)
{
	struct trtl_dev *trtl = f->private_data;
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t val, addr;
	int err, i;

	if (*offp + count >= TRTL_SMEM_MAX_SIZE)
		return -EINVAL;
	if (*offp % 4 || count % 4) {
		dev_warn(&trtl->dev, "Only word size access allowed\n");
		return -EINVAL;
	}

	addr = trtl->base_smem + (trtl->mod * TRTL_SMEM_MAX_SIZE) + *offp;
	for (i = 0; i < count; i += 4) {
		err = copy_from_user(&val, buf + i, 4);
		if (err) {
			dev_err(&trtl->dev,
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
static ssize_t trtl_read(struct file *f, char __user *buf,
			 size_t count, loff_t *offp)
{
	struct trtl_dev *trtl = f->private_data;
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t val, addr;
	int err, i;

	if (*offp + count >= TRTL_SMEM_MAX_SIZE)
		return -EINVAL;
	if (*offp % 4 || count % 4) {
		dev_warn(&trtl->dev, "Only word size access allowed\n");
		return -EINVAL;
	}

	addr = trtl->base_smem + *offp;
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
static int trtl_dev_simple_open(struct inode *inode, struct file *file)
{
	int m = iminor(inode);

	file->private_data = to_trtl_dev(minors[m]);

	return 0;
}

static const struct file_operations trtl_dev_fops = {
	.owner = THIS_MODULE,
	.open  = trtl_dev_simple_open,
	.read = trtl_read,
	.write = trtl_write,
	.llseek = generic_file_llseek,
	.unlocked_ioctl = trtl_ioctl,
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * DRIVER (un)LOADING  * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
 * Things to do after CPU device release
 */
static void trtl_cpu_release(struct device *dev)
{
	/*trtl_minor_put(dev);*/
}

/**
 * Things to do after HMQ device release
 */
static void trtl_hmq_release(struct device *dev)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);
	/*trtl_minor_put(dev);*/
	kfree(hmq->buf.mem);
}

#define TRTL_SLOT_CFG(_name, _val)                          \
	(1 << ((_val & MQUEUE_SLOT_STATUS_LOG2_##_name##_MASK) \
	       >> MQUEUE_SLOT_STATUS_LOG2_##_name##_SHIFT))
/**
 * It initializes and registers a HMQ device
 */
static int trtl_probe_hmq(struct trtl_dev *trtl, unsigned int slot,
			  unsigned int is_input)
{
	struct fmc_device *fmc = to_fmc_dev(trtl);
	struct trtl_hmq *hmq;
	uint32_t val;
	int err;

	hmq = is_input ? &trtl->hmq_in[slot] : &trtl->hmq_out[slot];

	hmq->index = slot;
	if (hmq_shared)
		hmq->flags |= TRTL_FLAG_HMQ_SHR_USR;

	err = trtl_minor_get(&hmq->dev, TRTL_HMQ);
	if (err)
		return err;

	err = dev_set_name(&hmq->dev, "trtl-%04x-hmq-%c-%02d",
			   fmc->device_id, is_input ? 'i':'o', hmq->index);
	if (err)
		return err;


	hmq->buf.ptr_w = 0;
	hmq->buf.ptr_r = 0;
	hmq->buf.size = hmq_default_buf_size;
	hmq->buf.mem = kzalloc(hmq->buf.size, GFP_KERNEL);
	if (!hmq->buf.mem)
		return -ENOMEM;

	init_waitqueue_head(&hmq->q_msg);
	hmq->dev.class = &trtl_cdev_class;
	hmq->dev.parent = &trtl->dev;
	hmq->dev.groups = trtl_hmq_groups;
	hmq->dev.release = trtl_hmq_release;
	err = device_register(&hmq->dev);
	if (err) {
		kfree(hmq->buf.mem);
		return err;
	}

	mutex_init(&hmq->mtx);
	mutex_init(&hmq->mtx_sync);
	INIT_LIST_HEAD(&hmq->list_msg_input);
	INIT_LIST_HEAD(&hmq->list_usr);

	if (is_input) { /* CPU input */
		hmq->flags |= TRTL_FLAG_HMQ_DIR;
		hmq->base_sr = trtl->base_hmq +	MQUEUE_BASE_IN(slot);
	} else { /* CPU output */
		hmq->base_sr = trtl->base_hmq +	MQUEUE_BASE_OUT(slot);
	}

	/* Get HMQ dimensions */
	val = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);
	hmq->max_width = TRTL_SLOT_CFG(WIDTH, val);
	hmq->max_depth = TRTL_SLOT_CFG(ENTRIES, val);
	hmq->buf.max_msg_size = hmq->max_width * 4;

	dev_dbg(&hmq->dev, " 0x%x -> %d %d\n",
		val, hmq->max_width, hmq->max_depth);
	spin_lock_init(&hmq->lock);
	/* Flush the content of the slot */
	fmc_writel(fmc, MQUEUE_CMD_PURGE,
		   hmq->base_sr + MQUEUE_SLOT_COMMAND);

	return 0;
}

/**
 * It initialize the WRNC device (device, CPUs, HMQs)
 */
int trtl_probe(struct fmc_device *fmc)
{
	struct trtl_dev *trtl;
	char tmp_name[128];
	int err, i;
	uint32_t tmp;

	/* Create a WRNC instance */
	trtl = devm_kzalloc(&fmc->dev, sizeof(struct trtl_dev), GFP_KERNEL);
	if (!trtl)
		return -ENOMEM;
	fmc_set_drvdata(fmc, trtl);

	err = fmc_scan_sdb_tree(fmc, 0x0);
	if (err < 0 && err != -EBUSY) {
		dev_err(fmc->hwdev, "SDB is missing\n");
		return err;
	}
	trtl->base_core = fmc_find_sdb_device(fmc->sdb, 0xce42, 0x90de, NULL);
	/* FIXME use SDB - <base> + <CSR offset> */
	trtl->base_csr  = trtl->base_core + 0xC000;
	trtl->base_smem = trtl->base_core + 0x10000;
	trtl->base_hmq  = trtl->base_core + BASE_HMQ;
	trtl->base_gcr  = trtl->base_hmq + MQUEUE_BASE_GCR;

	/* Register the device */
	err = dev_set_name(&trtl->dev, "trtl-%04x", fmc->device_id);
	if (err)
		return err;
	err = trtl_minor_get(&trtl->dev, TRTL_DEV);
	if (err)
		return err;
	trtl->dev.class = &trtl_cdev_class;
	trtl->dev.parent = &fmc->dev;
	trtl->dev.groups = trtl_dev_groups;
	trtl->dev.release = trtl_dev_release;
	err = device_register(&trtl->dev);
	if (err)
		return err;

	trtl->dbg_dir = debugfs_create_dir(dev_name(&trtl->dev), NULL);
	if (IS_ERR_OR_NULL(trtl->dbg_dir)) {
		pr_err("UAL: Cannot create debugfs\n");
		err = PTR_ERR(trtl->dbg_dir);
		goto out_dbg;
	}


	/* Get the Application ID */
	trtl->app_id = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_APP_ID);
	dev_info(&fmc->dev, "Application ID: 0x%08x\n", trtl->app_id);

	/* Get and check the number of COREs */
	trtl->n_cpu = fmc_readl(fmc,
				trtl->base_csr + WRN_CPU_CSR_REG_CORE_COUNT);
	if (trtl->n_cpu < 1 || trtl->n_cpu > TRTL_MAX_CPU) {
		dev_err(&fmc->dev, "invalid number of CPU (%d)\n", trtl->n_cpu);
		err = -EINVAL;
		goto out_n_cpu;
	}
	dev_info(&fmc->dev, "Detected %d CPUs\n", trtl->n_cpu);

	/* Pause all CPUs */
	trtl_cpu_enable_set(trtl, (1 << trtl->n_cpu) - 1);
	/* Reset all CPUs */
	trtl_cpu_reset_set(trtl, (1 << trtl->n_cpu) - 1);

	/* Configure CPUs */
	for (i = 0; i < trtl->n_cpu; ++i) {
		trtl->cpu[i].index = i;
		spin_lock_init(&trtl->cpu[i].lock);

		err = trtl_minor_get(&trtl->cpu[i].dev, TRTL_CPU);
		if (err)
			goto out_cpu;
		err = dev_set_name(&trtl->cpu[i].dev, "trtl-%04x-cpu-%02d",
				   fmc->device_id, trtl->cpu[i].index);
		if (err)
			goto out_cpu;
		trtl->cpu[i].dev.class = &trtl_cdev_class;
		trtl->cpu[i].dev.parent = &trtl->dev;
		trtl->cpu[i].dev.groups = trtl_cpu_groups;
		trtl->cpu[i].dev.release = trtl_cpu_release;
		err = device_register(&trtl->cpu[i].dev);
		if (err)
			goto out_cpu;
		snprintf(tmp_name, 128, "%s-dbg", dev_name(&trtl->cpu[i].dev));
		trtl->cpu[i].dbg_msg = debugfs_create_file(tmp_name, 0444,
							   trtl->dbg_dir,
							   &trtl->cpu[i],
							   &trtl_cpu_dbg_fops);
		if (IS_ERR_OR_NULL(trtl->cpu[i].dbg_msg))
			dev_err(&trtl->cpu[i].dev, "Cannot create debug interface\n");
	}

	/* Get and check the number of HMQ slots */
	tmp = fmc_readl(fmc, trtl->base_gcr + MQUEUE_GCR_SLOT_COUNT);
	trtl->n_hmq_in = tmp & MQUEUE_GCR_SLOT_COUNT_N_IN_MASK;
	trtl->n_hmq_out = (tmp & MQUEUE_GCR_SLOT_COUNT_N_OUT_MASK) >>
		MQUEUE_GCR_SLOT_COUNT_N_OUT_SHIFT;
	if (trtl->n_hmq_in + trtl->n_hmq_out >= TRTL_MAX_HMQ_SLOT) {
		dev_err(&fmc->dev, "trtl: invalid number of HMQ slots (in %d out %d)\n",
			 trtl->n_hmq_in, trtl->n_hmq_out);
		err = -EINVAL;
		goto out_n_slot;
	}
	dev_info(&fmc->dev, "Detected slots: %d input, %d output\n",
		trtl->n_hmq_in, trtl->n_hmq_out);

	/* Configure slots */
	for (i = 0; i < trtl->n_hmq_in; ++i) {
		err = trtl_probe_hmq(trtl, i, 1);
		if (err)
			goto out_hmq_in;
	}
	for (i = 0; i <  trtl->n_hmq_out; ++i) {
		err = trtl_probe_hmq(trtl, i, 0);
		if (err)
			goto out_hmq_out;
	}

	/*
	 * Great everything is configured properly, we can enable the interrupts
	 * now and start working.
	 */
	fmc->irq = trtl->base_core;
	err = fmc_irq_request(fmc, trtl_irq_handler,
			      (char *)dev_name(&trtl->dev),
			      0 /*VIC is used */);
	if (err) {
		dev_err(&trtl->dev,
			"Cannot request IRQ 0x%x - we'll not receive/send messages\n",
			fmc->irq);
	}

	/* Enable only necessary interrupts */
	trtl->irq_mask = 0;
	if (trtl->n_hmq_out)
		trtl->irq_mask |= (((1 << trtl->n_hmq_out) - 1)
				   << MQUEUE_GCR_IRQ_MASK_OUT_SHIFT);

	fmc_writel(fmc, trtl->irq_mask, trtl->base_gcr + MQUEUE_GCR_IRQ_MASK);
	tmp = fmc_readl(fmc, trtl->base_gcr + MQUEUE_GCR_IRQ_MASK);

	/* Enable debug interrupts */
	fmc->irq = trtl->base_core + 1;

	/* Enable debug interface interrupts only when we have space
	   to store it */
	err = fmc_irq_request(fmc,  trtl_irq_handler_debug,
			      (char *)dev_name(&trtl->cpu[i].dev),
			      0 /*VIC is used */);
	if (err) {
		dev_err(&trtl->dev,
			"Cannot request IRQ 0x%x - we'll not receive debug messages\n",
			fmc->irq);
	}

	if (dbg_max_msg > 0) {
		/* Enable interrupts only when we have a buffere where
		   store messages */
		fmc_writel(fmc, 0xFFFFFFFF/*(trtl->n_cpu - 1)*/,
			   trtl->base_csr + WRN_CPU_CSR_REG_DBG_IMSK);
	}

	/* Pin the carrier */
	if (!try_module_get(fmc->owner))
		goto out_mod;
	return 0;

out_mod:
out_hmq_out:
	while (--i)
		device_unregister(&trtl->hmq_out[i].dev);
	i = trtl->n_hmq_out;
out_hmq_in:
	while (--i)
		device_unregister(&trtl->hmq_in[i].dev);
out_n_slot:
	i = trtl->n_cpu;
out_cpu:
	while (--i)
		device_unregister(&trtl->cpu[i].dev);
out_n_cpu:
	debugfs_remove_recursive(trtl->dbg_dir);
out_dbg:
	device_unregister(&trtl->dev);
	return err;
}

/**
 * It remove the WRNC device (device, CPUs, HMQs) and free irq handler
 */
int trtl_remove(struct fmc_device *fmc)
{
	struct trtl_dev *trtl = fmc_get_drvdata(fmc);
	int i;

	fmc_writel(fmc, 0x0, trtl->base_gcr + MQUEUE_GCR_IRQ_MASK);
	fmc->irq = trtl->base_core;
	fmc_irq_free(fmc);

	fmc_writel(fmc, 0x0, trtl->base_csr + WRN_CPU_CSR_REG_DBG_IMSK);
	fmc->irq = trtl->base_core + 1;
	fmc_irq_free(fmc);

	debugfs_remove_recursive(trtl->dbg_dir);

	for (i = 0; i < trtl->n_cpu; ++i)
		device_unregister(&trtl->cpu[i].dev);

	for (i = 0; i < trtl->n_hmq_in; ++i)
		device_unregister(&trtl->hmq_in[i].dev);

	for (i = 0; i < trtl->n_hmq_out; ++i)
		device_unregister(&trtl->hmq_out[i].dev);

	/* FIXME cannot explain why, but without sleep the _kernel_ crash */
	msleep(50);
	device_unregister(&trtl->dev);

	/* Release the carrier */
	module_put(fmc->owner);

	return 0;
}


/**
 * List of device to match.
 */
static struct fmc_fru_id trtl_fru_id[] = {
	{
		.product_name = "wr-node-core"
	},
};

static struct fmc_driver trtl_dev_drv = {
	.version = FMC_VERSION,
	.driver.name = KBUILD_MODNAME,
	.probe = trtl_probe,
	.remove = trtl_remove,
	.id_table = {
		.fru_id = trtl_fru_id,
		.fru_id_nr = ARRAY_SIZE(trtl_fru_id),
	},
};


/**
 * Allocate resources for the driver. Char devices and FMC driver
 */
static int trtl_init(void)
{
	int err, i;

	for (i = 0; i < TRTL_MAX_CPU_MINORS; ++i)
		minors[i] = NULL;

	err = class_register(&trtl_cdev_class);
	if (err) {
		pr_err("%s: unable to register class\n", __func__);
		return err;
	}

	/* Allocate a char device region for devices, CPUs and slots */
	err = alloc_chrdev_region(&basedev, 0, TRTL_MAX_MINORS, "trtl");
	if (err) {
		pr_err("%s: unable to allocate region for %i minors\n",
		       __func__, TRTL_MAX_CPU_MINORS);
		goto out_all;
	}

	/* Register the device char-device */
	cdev_init(&cdev_dev, &trtl_dev_fops);
	cdev_dev.owner = THIS_MODULE;
	err = cdev_add(&cdev_dev, basedev, TRTL_MAX_CARRIER);
	if (err)
		goto out_cdev_dev;
	/* Register the cpu char-device */
	cdev_init(&cdev_cpu, &trtl_cpu_fops);
	cdev_cpu.owner = THIS_MODULE;
	err = cdev_add(&cdev_cpu, basedev + TRTL_MAX_CARRIER,
		       TRTL_MAX_CPU_MINORS);
	if (err)
		goto out_cdev_cpu;
	/* Register the hmq char-device */
	cdev_init(&cdev_hmq, &trtl_hmq_fops);
	cdev_cpu.owner = THIS_MODULE;
	err = cdev_add(&cdev_hmq,
		       basedev + TRTL_MAX_CARRIER + TRTL_MAX_CPU_MINORS,
		       TRTL_MAX_HMQ_MINORS);
	if (err)
		goto out_cdev_hmq;

	/* Register the FMC driver */
	err = fmc_driver_register(&trtl_dev_drv);
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
	unregister_chrdev_region(basedev, TRTL_MAX_MINORS);
out_all:
	class_unregister(&trtl_cdev_class);
	return err;
}


/**
 * Undo all the resource allocations
 */
static void trtl_exit(void)
{
	fmc_driver_unregister(&trtl_dev_drv);
	cdev_del(&cdev_cpu);
	unregister_chrdev_region(basedev,
				 TRTL_MAX_CPU_MINORS + TRTL_MAX_HMQ_MINORS);
	class_unregister(&trtl_cdev_class);
}

module_init(trtl_init);
module_exit(trtl_exit);

MODULE_AUTHOR("Federico Vaga <federico.vaga@cern.ch>");
MODULE_DESCRIPTION("Mock Turtle Linux Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(GIT_VERSION);

CERN_SUPER_MODULE;
