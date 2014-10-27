/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#include <linux/kernel.h>
#include <linux/module.h>
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
static struct device *minors[WRNC_MAX_MINORS];

static struct cdev cdev_hmq;

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

/**
 * Set the reset bit of the CPUs according to the mask
 */
static void wrnc_cpu_reset_set(struct wrnc_dev *wrnc, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_RESET);
	reg_val |= (mask & 0xFF);
	fmc_writel(fmc, reg_val, wrnc->base_csr + WRN_CPU_CSR_REG_RESET);
}

/**
 * Clear the reset bit of the CPUs according to the mask
 */
static void wrnc_cpu_reset_clr(struct wrnc_dev *wrnc, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_RESET);
	reg_val &= (~mask & 0xFF);
	fmc_writel(fmc, reg_val, wrnc->base_csr + WRN_CPU_CSR_REG_RESET);
}


/**
 * Set the reset bit of the CPUs according to the mask
 * Note that for the CPU 1 means pause
 */
static void wrnc_cpu_enable_set(struct wrnc_dev *wrnc, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);
	reg_val |= (mask & 0xFF);
	fmc_writel(fmc, reg_val, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);
}

/**
 * Clear the reset bit of the CPUs according to the mask
 * Note that for the CPU 0 means run
 */
static void wrnc_cpu_enable_clr(struct wrnc_dev *wrnc, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);
	reg_val &= (~mask & 0xFF);
	fmc_writel(fmc, reg_val, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);
}

/**
 * It returns the current enable status of the CPU
 */
static ssize_t wrnc_show_enable(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct wrnc_cpu *cpu = to_wrnc_cpu(dev);
	struct wrnc_dev *wrnc = to_wrnc_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);

	return sprintf(buf, "%d\n", !!(reg_val & (1 << cpu->index)));
}

/**
 * It enable or disable the CPU
 */
static ssize_t wrnc_store_enable(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct wrnc_cpu *cpu = to_wrnc_cpu(dev);
	struct wrnc_dev *wrnc = to_wrnc_dev(dev->parent);
	long val;

	if (strict_strtol(buf, 0, &val))
		return -EINVAL;

	if (val)
	        wrnc_cpu_enable_set(wrnc, (1 << cpu->index));
	else
	        wrnc_cpu_enable_clr(wrnc, (1 << cpu->index));

	return count;
}

/**
 * It returns the running status of the CPU
 */
static ssize_t wrnc_show_reset(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct wrnc_cpu *cpu = to_wrnc_cpu(dev);
	struct wrnc_dev *wrnc = to_wrnc_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_RESET);

	return sprintf(buf, "%d\n", !!(reg_val & (1 << cpu->index)));
}

/**
 * It run or pause the CPU
 */
static ssize_t wrnc_store_reset(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct wrnc_cpu *cpu = to_wrnc_cpu(dev);
	struct wrnc_dev *wrnc = to_wrnc_dev(dev->parent);
	long val;

	if (strict_strtol(buf, 0, &val))
		return -EINVAL;

	if (val)
	        wrnc_cpu_reset_set(wrnc, (1 << cpu->index));
	else
	        wrnc_cpu_reset_clr(wrnc, (1 << cpu->index));

	return count;
}

DEVICE_ATTR(enable, (S_IRUGO | S_IWUSR), wrnc_show_enable, wrnc_store_enable);
DEVICE_ATTR(reset, (S_IRUGO | S_IWUSR), wrnc_show_reset, wrnc_store_reset);

static struct attribute *wrnc_cpu_attr[] = {
	&dev_attr_enable.attr,
	&dev_attr_reset.attr,
	NULL,
};

static const struct attribute_group wrnc_cpu_group = {
	.attrs = wrnc_cpu_attr,
};

static const struct attribute_group *wrnc_cpu_groups[] = {
        &wrnc_cpu_group,
	NULL,
};

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


DEVICE_ATTR(application_id, S_IRUGO, wrnc_show_app_id, NULL);
DEVICE_ATTR(n_cpu, S_IRUGO, wrnc_show_n_cpu, NULL);

static struct attribute *wrnc_dev_attr[] = {
	&dev_attr_application_id.attr,
	&dev_attr_n_cpu.attr,
	NULL,
};

static const struct attribute_group wrnc_dev_group = {
	.attrs = wrnc_dev_attr,
};

static const struct attribute_group *wrnc_dev_groups[] = {
	&wrnc_dev_group,
	NULL,
};

static ssize_t wrnc_show_full(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);
	struct wrnc_dev *wrnc = to_wrnc_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t status;

	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);

	return sprintf(buf, "%d\n", !!(status & 0x1));
}
static ssize_t wrnc_show_empty(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);
	struct wrnc_dev *wrnc = to_wrnc_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t status;

	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);

	return sprintf(buf, "%d\n", !!(status & 0x2));
}
static ssize_t wrnc_show_count(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);
	struct wrnc_dev *wrnc = to_wrnc_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t status;

        status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);

	return sprintf(buf, "%d\n", ((status >> 2) & 0xFF));
}

DEVICE_ATTR(full, S_IRUGO, wrnc_show_full, NULL);
DEVICE_ATTR(empty, S_IRUGO, wrnc_show_empty, NULL);
DEVICE_ATTR(count, S_IRUGO, wrnc_show_count, NULL);

static struct attribute *wrnc_hmq_attr[] = {
	&dev_attr_full.attr,
	&dev_attr_empty.attr,
	&dev_attr_count.attr,
	NULL,
};

static const struct attribute_group wrnc_hmq_group = {
	.attrs = wrnc_hmq_attr,
};

static const struct attribute_group *wrnc_hmq_groups[] = {
	&wrnc_hmq_group,
	NULL,
};


static void wrnc_dev_release(struct device *dev)
{

}

static void wrnc_cpu_release(struct device *dev)
{
	//wrnc_minor_put(dev);
}

static void wrnc_hmq_release(struct device *dev)
{
	//wrnc_minor_put(dev);
}


/**
 * It loads a given application into the CPU memory
 */
static int wrnc_cpu_firmware_load(struct wrnc_cpu *cpu, void *fw_buf, size_t count, loff_t off)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(cpu->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t *fw = fw_buf, word, word_rb;
	int size, offset, i;

	if (off + count > 8192 * 4) {
		dev_err(&cpu->dev, "Cannot load firmware: size limit %d byte\n", 8192);
		return -ENOMEM;
	}

	/* Calculate code size in 32bit word*/
	size = (count + 3) / 4;
	offset = off / 4;

	/* Reset the CPU before overwrite its memory */
	wrnc_cpu_reset_set(wrnc, (1 << cpu->index));

	/* Select the CPU memory to write */
	fmc_writel(fmc, cpu->index, wrnc->base_csr + WRN_CPU_CSR_REG_CORE_SEL);

	/* Clean CPU memory */
	for (i = offset; i < 8192; ++i) { /* FIXME get size dynamically*/
		fmc_writel(fmc, i, wrnc->base_csr + WRN_CPU_CSR_REG_UADDR);
		fmc_writel(fmc, 0, wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
	}

	/* Load the firmware */
	for (i = offset; i < size; ++i) {
		word = cpu_to_be32(fw[i]);
		fmc_writel(fmc, i, wrnc->base_csr + WRN_CPU_CSR_REG_UADDR);
		fmc_writel(fmc, word, wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
		word_rb = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
		if (word != word_rb) {
			dev_err(&cpu->dev, "failed to load firmware\n");
		        return -EFAULT;
		}
	}

	return 0;
}

static int wrnc_cpu_firmware_dump(struct wrnc_cpu *cpu, void *fw_buf, size_t count, loff_t off)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(cpu->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t *fw = fw_buf, word;
	int size, offset, i;

	if (off + count > 8192 * 4) {
		dev_err(&cpu->dev, "Cannot load firmware: size limit %d byte\n", 8192);
		return -ENOMEM;
	}

	/* Calculate code size in 32bit word*/
	size = (count + 3) / 4;
	offset = off / 4;

	/* Select the CPU memory to write */
	fmc_writel(fmc, cpu->index, wrnc->base_csr + WRN_CPU_CSR_REG_CORE_SEL);

	/* Dump the firmware */
	for (i = 0; i < size; ++i) {
		fmc_writel(fmc, i + offset, wrnc->base_csr + WRN_CPU_CSR_REG_UADDR);
		word = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
		fw[i] = be32_to_cpu(word);
	}

	return 0;
}

static int wrnc_dev_simple_open(struct inode *inode, struct file *file)
{
	int m = iminor(inode);

	file->private_data = to_wrnc_dev(minors[m]);

	return 0;
}

static const struct file_operations wrnc_dev_fops = {
	.owner = THIS_MODULE,
	.open  = wrnc_dev_simple_open,
};


static int wrnc_cpu_simple_open(struct inode *inode, struct file *file)
{
	int m = iminor(inode);

	file->private_data = to_wrnc_cpu(minors[m]);

	return 0;
}

/**
 * It writes a given firmware into a CPU
 */
static ssize_t wrnc_cpu_write(struct file *f, const char __user *buf,
			      size_t count, loff_t *offp)
{
	struct wrnc_cpu *cpu = f->private_data;
	void *lbuf;
	int err;

	pr_info("%s:%d %d %d\n", __func__, __LINE__, count, *offp);
	if (!count)
		return -EINVAL;

	lbuf = vmalloc(count);
	if (!lbuf)
		return -ENOMEM;
	if (copy_from_user(lbuf, buf, count)) {
		err = -EFAULT;
		goto out_cpy;
	}

	err = wrnc_cpu_firmware_load(cpu, lbuf, count, *offp);
	if (err)
		goto out_load;

	*offp += count;
	pr_info("%s:%d %d %d\n", __func__, __LINE__, count, *offp);

out_load:
out_cpy:
	vfree(lbuf);
	return err ? err : count;
}

/**
 * It reads the firmware from a CPU
 */
static ssize_t wrnc_cpu_read(struct file *f, char __user *buf,
			     size_t count, loff_t *offp)
{
	struct wrnc_cpu *cpu = f->private_data;
	void *lbuf;
	int err;

	pr_info("%s:%d %d %d\n", __func__, __LINE__, count, *offp);
	lbuf = vmalloc(count);
	if (!lbuf)
		return -ENOMEM;

	err = wrnc_cpu_firmware_dump(cpu, lbuf, count, *offp);
	if (err)
		goto out_dmp;

	if (copy_to_user(buf, lbuf, count)) {
		err = -EFAULT;
		goto out_cpy;
	}

	*offp += count;
	pr_info("%s:%d %d %d\n", __func__, __LINE__, count, *offp);

out_cpy:
out_dmp:
	vfree(lbuf);
	return err ? err : count;
}

static const struct file_operations wrnc_cpu_fops = {
	.owner = THIS_MODULE,
	.open  = wrnc_cpu_simple_open,
	.write  = wrnc_cpu_write,
	.read = wrnc_cpu_read,
	.llseek = generic_file_llseek,
};

static int wrnc_hmq_simple_open(struct inode *inode, struct file *file)
{
	int m = iminor(inode);

	pr_info("%s:%d\n", __func__, __LINE__);

	file->private_data = to_wrnc_hmq(minors[m]);

	return 0;
}

static ssize_t wrnc_hmq_write(struct file *f, const char __user *buf,
			      size_t count, loff_t *offp)
{
	struct wrnc_hmq *hmq = f->private_data;

	pr_info("%s:%d\n", __func__, __LINE__);
	if (!(hmq->flags & WRNC_FLAG_HMQ_DIR)) {
		dev_err(&hmq->dev, "cannot write on an output queue\n");
		return -EFAULT;
	}

	if (count % sizeof(struct wrnc_msg)) {
		dev_err(&hmq->dev, "we can write only entire messages\n");
		return -EINVAL;
	}

	return count;
}

static void wrnc_message_push(struct wrnc_hmq *hmq, struct wrnc_msg *msg)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	unsigned long flags;
	int i;

	spin_lock_irqsave(&hmq->lock, flags);
	fmc_writel(fmc, MQUEUE_CMD_CLAIM, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	for (i = 0; i < msg->datalen; ++i) {
		fmc_writel(fmc, msg->data[i],
			   hmq->base_sr + MQUEUE_SLOT_DATA_START + i * 4);
	}
	fmc_writel(fmc, MQUEUE_CMD_CLAIM, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	spin_unlock_irqrestore(&hmq->lock, flags);
}

static struct wrnc_msg *wrnc_message_pop(struct wrnc_hmq *hmq)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	struct wrnc_msg *msg;
	unsigned long flags;
	uint32_t status;
	char str[128];
	int i;

	msg = kmalloc(sizeof(struct wrnc_msg), GFP_KERNEL);
	if (msg)
		return ERR_PTR(-ENOMEM);

	spin_lock_irqsave(&hmq->lock, flags);
	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);
	msg->datalen = (status >> 16) & 0xFF;
	for (i = 0; i < msg->datalen; ++i) {
		msg->data[i] = fmc_readl(fmc,
				hmq->base_sr + MQUEUE_SLOT_DATA_START + i * 4);
	}
	fmc_writel(fmc, MQUEUE_CMD_DISCARD, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	spin_unlock_irqrestore(&hmq->lock, flags);

	if (msg->data[0] == 0xdeadbeef) {
	        for (i = 0; i < 128 - 1; i++) {
			str[i] = msg->data[i + 1];
		}
		str[i] = 0;
		dev_err(&hmq->dev, "Cannot retrieve message: %s\n", str);
		kfree(msg);
		msg = NULL;
	}

	return msg;
}

static int wrnc_ioctl_msg_sync(struct wrnc_hmq *hmq, void __user *uarg)
{
	struct wrnc_msg_element *msgel;
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct wrnc_msg_sync msg;
	struct wrnc_hmq *hmq_out;
	int err = 0;

	/* Copy the message from user space*/
	err = copy_from_user(&msg, uarg, sizeof(struct wrnc_msg_sync));
	if (err)
	        return err;

	if (hmq->index != msg.index_in) {
		dev_warn(&hmq->dev,
			 "cannot enqueue messages on other slots\n");
		return -EINVAL;
	}
	if (msg.index_out >= wrnc->n_hmq_out) {
		dev_err(&hmq->dev, "un-existent slot %d\n", msg.index_out);
		return -EINVAL;
	}
	hmq_out = &wrnc->hmq_out[msg.index_out];

	/*
	 * Wait until the message queue is empty so we can safely enqueue
	 * the synchronous message. Then get the mutex to avoid other process
	 * to write
	 */
	wait_event_interruptible(hmq->q_msg, list_empty(&hmq->list_msg));
	mutex_lock(&hmq->mtx);

	/*
	 * Wait for the synchronous answer. then get the mutex to avoit other
	 * processes to read
	 */
	wait_event_interruptible(hmq_out->q_msg, !list_empty(&hmq_out->list_msg));
	mutex_lock(&hmq_out->mtx);

	/* Send the message */
	wrnc_message_push(hmq, &msg.msg);

	/* We have at least one message in the buffer, return it */
	spin_lock(&hmq_out->lock);
	msgel = list_entry(hmq_out->list_msg.next, struct wrnc_msg_element, list);
	list_del(&msgel->list);
	spin_unlock(&hmq_out->lock);

	mutex_unlock(&hmq_out->mtx);
	mutex_unlock(&hmq->mtx);

	/* Copy the answer message back to user space */
        memcpy(&msg.msg, msgel->msg, sizeof(struct wrnc_msg));
	err = copy_to_user(uarg, &msg, sizeof(struct wrnc_msg_sync));
	kfree(msgel->msg);
	kfree(msgel);

	return err;
}

static long wrnc_hmq_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct wrnc_hmq *hmq = f->private_data;
	void __user *uarg = (void __user *)arg;
	int err = 0;

	pr_info("%s:%d\n", __func__, __LINE__);
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

	switch (cmd) {
	case WRNC_IOCTL_MSG_SYNC:
		err = wrnc_ioctl_msg_sync(hmq, uarg);
		break;
	default:
		pr_warn("ual: invalid ioctl command %d\n", cmd);
		return -EINVAL;
	}

	return err;
}


/**
 * It returns to user space messages from an output hmq
 */
static ssize_t wrnc_hmq_read(struct file *f, char __user *buf,
			     size_t count, loff_t *offp)
{
	struct wrnc_hmq *hmq = f->private_data;
	struct wrnc_msg_element *msgel;
	unsigned int i, n;

	pr_info("%s:%d\n", __func__, __LINE__);
	if (hmq->flags & WRNC_FLAG_HMQ_DIR) {
		dev_err(&hmq->dev, "cannot read from an input queue\n");
		return -EFAULT;
	}
	pr_info("%s:%d\n", __func__, __LINE__);
	if (count % sizeof(struct wrnc_msg)) {
		dev_err(&hmq->dev, "we can read only entire messages\n");
		return -EINVAL;
	}
	n = count / sizeof(struct wrnc_msg);


	count = 0;
	/* read as much as we can */
	for (i = 0; i < n; ++i) {
		if (list_empty(&hmq->list_msg)) {
			*offp = 0;
			break;
		}

		spin_lock(&hmq->lock);
		msgel = list_entry(hmq->list_msg.next, struct wrnc_msg_element, list);
		list_del(&msgel->list);
		spin_unlock(&hmq->lock);

		count = i * sizeof(struct wrnc_msg);
		if (copy_to_user(buf + count, msgel->msg, sizeof(struct wrnc_msg)))
			return -EFAULT;
		kfree(msgel->msg);
		kfree(msgel);
	}

	*offp += count;

	return count;
}

static unsigned int wrnc_hmq_poll(struct file *f, struct poll_table_struct *w)
{
	struct wrnc_hmq *hmq = f->private_data;
	unsigned int ret = 0;

	pr_info("%s:%d\n", __func__, __LINE__);
	poll_wait(f, &hmq->q_msg, w);

	/* check if there are interrupts to notify */
	if (!list_empty(&hmq->list_msg)) {
		if (hmq->flags & WRNC_FLAG_HMQ_DIR) /* CPU input */
			ret |= POLLOUT | POLLWRNORM;
		else /* CPU output */
			ret |= POLLIN | POLLRDNORM;
	}

	return ret;
}

static const struct file_operations wrnc_hmq_fops = {
	.owner = THIS_MODULE,
	.open  = wrnc_hmq_simple_open,
	.write  = wrnc_hmq_write,
	.read = wrnc_hmq_read,
	.unlocked_ioctl = wrnc_hmq_ioctl,
	.poll = wrnc_hmq_poll,
};

/**
 * It handles an input interrupts. The CPU is waiting for input data, so
 * we should feed the CPU if we have something in our local buffer.
 */
static void wrnc_irq_handler_input(struct wrnc_hmq *hmq)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	struct wrnc_msg_element *msgel;
	unsigned long flags;
	uint32_t mask;

	pr_info("%s:%d\n", __func__, __LINE__);
	spin_lock_irqsave(&hmq->lock, flags);
	if (list_empty(&hmq->list_msg)) {
		/* We don't have nothing to send, disable the interrupts */
		mask = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
		mask &= ~(1 << hmq->index);
		fmc_writel(fmc, mask, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
		spin_unlock_irqrestore(&hmq->lock, flags);
		return;
	}

	/* Retrieve and send the first message */
        msgel = list_entry(hmq->list_msg.next, struct wrnc_msg_element, list);
	list_del(&msgel->list);
	wrnc_message_push(hmq, msgel->msg);

	spin_unlock_irqrestore(&hmq->lock, flags);

	/* Release resources */
	kfree(msgel->msg);
	kfree(msgel);
}

/**
 * It handles an output interrupt. It means that the CPU is outputting
 * data for us, so we must read it.
 */
static void wrnc_irq_handler_output(struct wrnc_hmq *hmq)
{
	struct wrnc_msg_element *msgel;
	unsigned long flags;

	pr_info("%s:%d\n", __func__, __LINE__);
	/* Allocate space for the incoming message */
	msgel = kmalloc(sizeof(struct wrnc_msg_element), GFP_KERNEL);
	if (!msgel) {
		dev_err(&hmq->dev,
			"Cannot allocate memory for a new message\n");
		return;
	}

	msgel->msg = wrnc_message_pop(hmq);
	if (IS_ERR_OR_NULL(msgel->msg)) {
		kfree(msgel);
		return;
	}

	/* We have a valid message, store it */
	spin_lock_irqsave(&hmq->lock, flags);
	list_add_tail(&msgel->list, &hmq->list_msg);
	spin_unlock_irqrestore(&hmq->lock, flags);

	/* Wake up processes waiting for this */
	wake_up_interruptible(&hmq->q_msg);
}

/**
 * It handles HMQ interrupts. It checks if any of the slot has a pending
 * interrupt. If the interrupt is pending it will handle it, otherwise it
 * checks the next slot. In order to optimize the interrupt management,
 * after a first run through all slots it checks again if any interrupt
 * occurse while handling another one.
 */
irqreturn_t wrnc_irq_handler(int irq_core_base, void *arg)
{
	struct fmc_device *fmc = arg;
	struct wrnc_dev *wrnc = fmc_get_drvdata(fmc);
	uint32_t status;
	int i = -1;

	pr_info("%s:%d\n", __func__, __LINE__);
	/* Get the source of interrupt */
	status = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_SLOT_STATUS);
		wrnc->base_gcr + MQUEUE_GCR_SLOT_STATUS, status);
dispatch_irq:
        while (status && i < WRNC_MAX_HMQ_SLOT) {
		++i;
		if (!(status & 0x1))
			continue;

		if (i >= MAX_MQUEUE_SLOTS)
			wrnc_irq_handler_input(&wrnc->hmq_in[i - MAX_MQUEUE_SLOTS]);
		else
			wrnc_irq_handler_output(&wrnc->hmq_out[i]);

		/* Clear handled interrupts */
		status >>= 1;
	}
	/*
	 * check if other interrupts occurs in the meanwhile
	 */
	status = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_SLOT_STATUS);
	if (status)
		goto dispatch_irq;

	fmc->op->irq_ack(fmc);

	return IRQ_HANDLED;
}

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

	return 0;
}

/**
 * It initialize the WRNC device
 */
int wrnc_probe(struct fmc_device *fmc)
{
	struct wrnc_dev *wrnc;
	int err, i;
	uint32_t tmp;

	pr_info("%s:%d\n", __func__, __LINE__);
	/* Create a WRNC instance */
	wrnc = devm_kzalloc(&fmc->dev, sizeof(struct wrnc_dev), GFP_KERNEL);
	if (!wrnc)
		return -ENOMEM;
	fmc_set_drvdata(fmc, wrnc);

	/* FIXME use SDB - <base> + <CSR offset> */
	wrnc->base_core = 0xC0000;
	wrnc->base_csr = wrnc->base_core + 0x10000;
	wrnc->base_hmq = wrnc->base_core + BASE_HMQ;
	wrnc->base_gcr = wrnc->base_hmq + MQUEUE_BASE_GCR;

	/* Register the device */
	err = dev_set_name(&wrnc->dev,"wrnc-%04x", fmc->device_id);
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
	wrnc->n_cpu = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_CORE_COUNT);
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
		/* TODO add fops to r/w firmware */
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
	fmc->irq = 0xC0000;//fmc_find_sdb_device(fmc->sdb, 0xce42, 0x13, NULL);
	err = fmc->op->irq_request(fmc, wrnc_irq_handler, (char *)dev_name(&wrnc->dev),
				   0 /*VIC is used */);
	if (err) {
		dev_err(&wrnc->dev,
			"Cannot request IRQ 0x%x - we'll not receive/send messages\n",
			fmc->irq);
	}
	tmp = (((1 << wrnc->n_hmq_in) - 1) << MQUEUE_GCR_IRQ_MASK_IN_SHIFT);
	tmp |= (1 << wrnc->n_hmq_out) - 1;
	//fmc_writel(fmc, tmp, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);

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
 * It remove the WRNC device
 */
int wrnc_remove(struct fmc_device *fmc)
{
	struct wrnc_dev *wrnc = fmc_get_drvdata(fmc);
	int i;

	fmc_writel(fmc, 0x0, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
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

static struct fmc_fru_id wrnc_fru_id[] = {
	{
		//.product_name = "white-rabbit-node-core",
		.product_name = "FmcTdc1ns5cha"
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

static int wrnc_init(void)
{
	int err, i;

	pr_info("%s:%d\n", __func__, __LINE__);
	for (i = 0; i < WRNC_MAX_CPU_MINORS; ++i) {
		minors[i] = NULL;
	}

	err = class_register(&wrnc_cdev_class);
	if (err) {
		pr_err("%s: unable to register class\n", __func__);
	        return err;
	}
	/* Allocate a char device region for CPU and slots */
	err = alloc_chrdev_region(&basedev, 0, WRNC_MAX_MINORS, "wrnc");
	if (err) {
		pr_err("%s: unable to allocate region for %i minors\n",
		       __func__, WRNC_MAX_CPU_MINORS);
	        goto out_all;
	}

	/* Register the char device */
	cdev_init(&cdev_dev, &wrnc_dev_fops);
	cdev_dev.owner = THIS_MODULE;
	err = cdev_add(&cdev_dev, basedev, WRNC_MAX_CARRIER);
	if (err)
		goto out_cdev_dev;
	cdev_init(&cdev_cpu, &wrnc_cpu_fops);
	cdev_cpu.owner = THIS_MODULE;
	err = cdev_add(&cdev_cpu, basedev + WRNC_MAX_CARRIER,
		       WRNC_MAX_CPU_MINORS);
	if (err)
		goto out_cdev_cpu;
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

static void wrnc_exit(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
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
