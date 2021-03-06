/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/circ_buf.h>

#include <linux/fmc.h>


#include <hw/mockturtle_queue.h>

#include "mockturtle-drv.h"

int hmq_default_buf_size = 8192; /**< default buffer size in byte */
module_param_named(slot_buffer_size, hmq_default_buf_size, int, 0444);
MODULE_PARM_DESC(slot_buffer_size, "Default buffer size in byte.");

int hmq_max_con = 8; /**< Maximum number connection for each slot */
module_param_named(max_slot_con, hmq_max_con, int, 0644);
MODULE_PARM_DESC(max_slot_con, "Maximum number connection for each slot.");

int hmq_sync_timeout = 1000; /**< Milli-seconds to wait for a synchronous answer */
module_param_named(sync_timeout, hmq_sync_timeout, int, 0644);
MODULE_PARM_DESC(sync_timeout, "Milli-seconds to wait for a synchronous answer.");

int hmq_shared = 0; /**< Maximum number connection for each slot */
module_param_named(slot_share, hmq_shared, int, 0444);
MODULE_PARM_DESC(slot_share, "Set if by default slot are shared or not.");

static int hmq_in_irq = 0;
module_param_named(hmq_in_irq_enable, hmq_in_irq, int, 0444);
MODULE_PARM_DESC(hmq_in_irq, "Set it if you want to use interrupts to communicate from host to the cores. Default 0");

static int hmq_in_no_irq_wait = 10;
module_param_named(hmq_in_no_irq_wait_us, hmq_in_no_irq_wait, int, 0444);
MODULE_PARM_DESC(hmq_in_no_irq_wait, "Time (us) to wait after sending a message from the host to the core in a no-interrupt context. Default 10us");

static int hmq_max_irq_loop = 5;
module_param_named(max_irq_loop, hmq_max_irq_loop, int, 0644);
MODULE_PARM_DESC(max_irq_loop, "Maximum number of messages to read per interrupt per hmq");

static int trtl_message_push(struct trtl_hmq *hmq, void *buf,
			     unsigned int size,  uint32_t *seq);

/**
 * It applies filters on a given message.
 */
static int trtl_hmq_filter_check(struct trtl_hmq_user *user, void *buffer)
{
	struct trtl_msg_filter_element *fltel, *tmp;
	unsigned int passed = 1;
	uint32_t word, *data = buffer;

	spin_lock(&user->lock_filter);
	list_for_each_entry_safe (fltel, tmp, &user->list_filters, list) {
		/* If one of the previous filter failed, then stop */
		if (!passed)
			break;

		word = data[fltel->filter.word_offset];
		switch(fltel->filter.operation) {
		case TRTL_MSG_FILTER_AND:
			word &= fltel->filter.mask;
			break;
		case TRTL_MSG_FILTER_OR:
			word |= fltel->filter.mask;
			break;
		case TRTL_MSG_FILTER_EQ:
			break;
#if 0 /* FIXME not clear from specification what NOT should do*/
		case TRTL_MSG_FILTER_NOT:
			word ~= word;
			break;
#endif
		}
		if (word != fltel->filter.value )
			passed = 0;
	}
	spin_unlock(&user->lock_filter);

	return passed;
}


/**
 * It return 1 if the message quque slot is full
 */
static ssize_t trtl_show_full(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);
	struct trtl_dev *trtl = to_trtl_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t status;

	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);

	return sprintf(buf, "%d\n", !!(status & 0x1));
}


/**
 * It return 1 if the message quque slot is empty
 */
static ssize_t trtl_show_empty(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);
	struct trtl_dev *trtl = to_trtl_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t status;

	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);

	return sprintf(buf, "%d\n", !!(status & 0x2));
}


/**
 * It returns the number of messages in the WRNC queue
 */
static ssize_t trtl_show_count(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);
	struct trtl_dev *trtl = to_trtl_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t status;

	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);

	return sprintf(buf, "%d\n", ((status >> 2) & 0xFF));
}


/**
 * It returns the maximum buffer size
 */
static ssize_t trtl_show_buffer_size(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);

	return sprintf(buf, "%d\n", hmq->buf.size);
}

static ssize_t trtl_store_buffer_size(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);
	struct trtl_hmq_user *usr, *tmp;
	void *newbuf;
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	if (val == hmq->buf.size)
		return count;

	if (val < hmq->buf.max_msg_size) {
		dev_err(dev,
			"Buffer size (%ld) must at least double the size of the maximum message sized (%d)\n",
			val, hmq->buf.max_msg_size);
		return -EINVAL;
	}

	if ((val - 1) & val) {
		dev_err(dev, "Buffer size (%ld) must be power of two\n", val);
		return -EINVAL;
	}

	newbuf = kzalloc(val, GFP_KERNEL);
	if (!newbuf) {
		dev_err(dev, "Cannot allocate new buffer (%ld)\n", val);
		return -ENOMEM;
	}

	spin_lock(&hmq->lock);
	kfree(hmq->buf.mem);
	hmq->buf.mem = newbuf;
	hmq->buf.size = val;
	hmq->buf.ptr_w = 0;
	hmq->buf.ptr_r = 0;

	list_for_each_entry_safe(usr, tmp, &hmq->list_usr, list) {
		spin_lock(&usr->lock);
		usr->ptr_r = 0;
		spin_unlock(&usr->lock);
	}
	spin_unlock(&hmq->lock);

	return count;
}


/**
 * It returns the maximum number of messages in the WRNC queue
 */
static ssize_t trtl_show_count_max_hw(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);

	return sprintf(buf, "%d\n", hmq->max_depth);
}


/**
 * It returns the maximum number bytes per message
 */
static ssize_t trtl_show_width_max(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);

	return sprintf(buf, "%d\n", hmq->max_width * 4);
}

/**
 * Show the current share status of the HMQ slot
 */
static ssize_t trtl_show_share(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);

	return sprintf(buf, "%d\n", !!(hmq->flags & TRTL_FLAG_HMQ_SHR_USR));
}


/**
 * Set if the char-device are in shared mode or not
 */
static ssize_t trtl_store_share(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	/*
	 * If the status is the same, than there is nothing to do. This
	 * control sounds useless but it save code in user-space and it
	 * allows a proper error management
	 */
	if (val == (!!(hmq->flags & TRTL_FLAG_HMQ_SHR_USR)))
		return count;

	/* You cannot configure while the HMQ is in use */
	if (hmq->n_user > 0)
		return -EBUSY;

	spin_lock(&hmq->lock);
	if (val)
		hmq->flags |= TRTL_FLAG_HMQ_SHR_USR;
	else
		hmq->flags &= ~TRTL_FLAG_HMQ_SHR_USR;
	spin_unlock(&hmq->lock);

	return count;
}


/**
 * It returns the maximum number bytes per message
 */
static ssize_t trtl_show_total(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct trtl_hmq *hmq = to_trtl_hmq(dev);

	return sprintf(buf, "%d\n", hmq->stats.count);
}


DEVICE_ATTR(full, S_IRUGO, trtl_show_full, NULL);
DEVICE_ATTR(empty, S_IRUGO, trtl_show_empty, NULL);
DEVICE_ATTR(count_hw, S_IRUGO, trtl_show_count, NULL);
DEVICE_ATTR(buffer_size, S_IRUGO | S_IWUSR | S_IWGRP,
	    trtl_show_buffer_size, trtl_store_buffer_size);
DEVICE_ATTR(count_max_hw, S_IRUGO, trtl_show_count_max_hw, NULL);
DEVICE_ATTR(width_max, S_IRUGO, trtl_show_width_max, NULL);
DEVICE_ATTR(shared_by_users, (S_IRUGO | S_IWUSR | S_IWGRP |  S_IWOTH),
	    trtl_show_share, trtl_store_share);
DEVICE_ATTR(total_messages, S_IRUGO, trtl_show_total, NULL);

static struct attribute *trtl_hmq_attr[] = {
	&dev_attr_full.attr,
	&dev_attr_empty.attr,
	&dev_attr_count_hw.attr,
	&dev_attr_buffer_size.attr,
	&dev_attr_count_max_hw.attr,
	&dev_attr_width_max.attr,
	&dev_attr_shared_by_users.attr,
	&dev_attr_total_messages.attr,
	NULL,
};

static const struct attribute_group trtl_hmq_group = {
	.attrs = trtl_hmq_attr,
};

const struct attribute_group *trtl_hmq_groups[] = {
	&trtl_hmq_group,
	NULL,
};



/**
 * It simply opens a HMQ device
 */
static int trtl_hmq_open(struct inode *inode, struct file *file)
{
	struct trtl_hmq_user *user;
	struct trtl_hmq *hmq;
	unsigned long flags;
	int m = iminor(inode);

	hmq = to_trtl_hmq(minors[m]);

	if (list_empty(&hmq->list_usr) || (hmq->flags & TRTL_FLAG_HMQ_SHR_USR)) {
		user = kzalloc(sizeof(struct trtl_hmq_user), GFP_KERNEL);
		if (!user)
			return -ENOMEM;

		user->hmq = hmq;
		spin_lock_init(&user->lock);
		spin_lock_init(&user->lock_filter);
		INIT_LIST_HEAD(&user->list_filters);

		/* Add new user to the list */
		spin_lock_irqsave(&hmq->lock, flags);
		list_add(&user->list, &hmq->list_usr);
		hmq->n_user++;
		spin_unlock_irqrestore(&hmq->lock, flags);
	} else {
		/*
		 * It is NOT empty and it is NOT shared. So it means that there is
		 * a single instance shared by different user-space processes
		 */
		spin_lock_irqsave(&hmq->lock, flags);
		/* Use the same instance for all the consumers */
		user = list_entry(hmq->list_usr.next,
				   struct trtl_hmq_user, list);
		hmq->n_user++;
		spin_unlock_irqrestore(&hmq->lock, flags);
	}

	spin_lock_irqsave(&hmq->lock, flags);
	/* Point to the current position in buffer */
	user->ptr_r = hmq->buf.ptr_w;
	spin_unlock_irqrestore(&hmq->lock, flags);

	file->private_data = user;

	return 0;
}

static int trtl_hmq_release(struct inode *inode, struct file *f)
{
	struct trtl_hmq_user *user = f->private_data;
	struct trtl_hmq *hmq = user->hmq;
	unsigned long flags;


	/* Remove user from the list */
	spin_lock_irqsave(&hmq->lock, flags);
	hmq->n_user--;

	if (hmq->flags & TRTL_FLAG_HMQ_SHR_USR || hmq->n_user == 0) {
		list_del(&user->list);
		kfree(user);
	}

	/* Reset the default shared status */
	if (hmq->n_user == 0) {
		if (hmq_shared)
			hmq->flags |= TRTL_FLAG_HMQ_SHR_USR;
		else
			hmq->flags &= ~TRTL_FLAG_HMQ_SHR_USR;
	}
	spin_unlock_irqrestore(&hmq->lock, flags);

	return 0;
}

/**
 * It writes message in the drive message queue. The messages will be sent on
 * IRQ signal
 * @TODO to be tested! WRTD is using only sync messages
 */
static ssize_t trtl_hmq_write(struct file *f, const char __user *buf,
			      size_t count, loff_t *offp)
{
	struct trtl_hmq_user *user = f->private_data;
	struct trtl_hmq *hmq = user->hmq;
	struct trtl_dev *trtl = to_trtl_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	struct trtl_msg msg;
	unsigned long flags;
	unsigned int i, n;
	const char __user *curbuf = buf;
	uint32_t mask, seq;
	int err = 0;

	if (!(hmq->flags & TRTL_FLAG_HMQ_DIR)) {
		dev_err(&hmq->dev, "cannot write on an output queue\n");
		return -EFAULT;
	}

	if (count % sizeof(struct trtl_msg)) {
		dev_err(&hmq->dev, "we can write only entire messages\n");
		return -EINVAL;
	}


	/* Get number of free slots */
	n = count / sizeof(struct trtl_msg);
	count = 0;
	mutex_lock(&hmq->mtx);

	for (i = 0; i < n; i++, curbuf += sizeof(struct trtl_msg)) {
		if (copy_from_user(&msg, curbuf, sizeof(struct trtl_msg))) {
			err = -EFAULT;
			break;
		}

		if (msg.datalen * 4 >= hmq->buf.max_msg_size) {
			dev_err(&hmq->dev,
				"Cannot send %d bytes, the maximum size is %d bytes\n",
				msg.datalen * 4, hmq->buf.max_msg_size);
			err = -EINVAL;
			break;
		}

		/* Enqueue messages if we are going to send them using
		   interrupts; otherwise sen them immediately */
		if (hmq_in_irq) {
			if (CIRC_SPACE(hmq->buf.ptr_w, hmq->buf.ptr_r,
				       hmq->buf.size) == 0) {
				/* No more space left */
				break;
			}
			spin_lock_irqsave(&hmq->lock, flags);
			memcpy(hmq->buf.mem + hmq->buf.ptr_w, msg.data,
			       hmq->buf.max_msg_size);
		        hmq->buf.ptr_w += hmq->buf.max_msg_size;
			if (hmq->buf.ptr_w >= hmq->buf.size) {
				hmq->buf.ptr_w = 0;
			}
			spin_unlock_irqrestore(&hmq->lock, flags);
		} else {
			spin_lock_irqsave(&hmq->lock, flags);
			err = trtl_message_push(hmq, msg.data,
						msg.datalen * 4, &seq);
			spin_unlock_irqrestore(&hmq->lock, flags);
			if (err)
				break;
			/* wait to give time to the core to get and consume
			   the message before sending the next one */
			ndelay(hmq_in_no_irq_wait * 1000);
		}

	}
	mutex_unlock(&hmq->mtx);

	/* Enable interrupts for CPU input SLOT */
	if (hmq_in_irq) {
		mask = fmc_readl(fmc, trtl->base_gcr + MQUEUE_GCR_IRQ_MASK);
		mask |= (1 << (hmq->index + MQUEUE_GCR_IRQ_MASK_IN_SHIFT));
		fmc_writel(fmc, mask, trtl->base_gcr + MQUEUE_GCR_IRQ_MASK);
	}

	/* Update counter */
	count = i * sizeof(struct trtl_msg);
	*offp += count;

	/*
	 * If `count` is not 0, it means that we saved at least one message, even
	 * if we got an error on the second message. So, in this case notifiy the
	 * user space about how many messages where stored and ignore errors.
	 * Return the error only when we did not save any message.
	 *
	 * I choosed this solution because we do not have any dangerous error here,
	 * and it simplify the code.
	 */
	return count ? count : err;
}

/**
 * It writes a message to a FPGA HMQ. Note that you have to take
 * the HMQ spinlock before call this function
 * @param[in] hmq queue where send the message
 * @param[in] buf data buffer to send
 * @param[in] size buffer size
 * @param[out] the sequence number used to send the message
 */
static int trtl_message_push(struct trtl_hmq *hmq, void *buf,
			     unsigned int size, uint32_t *seq)
{
	struct trtl_dev *trtl = to_trtl_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t freeslot, *data = buf;
	int i;

	if (size > hmq->buf.max_msg_size) {
		dev_err(&hmq->dev,
			"The message (%d bytes) does not fit in the maximum message size (%d bytes)\n",
			size, hmq->buf.max_msg_size);
		return -EINVAL;
	}
	freeslot = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS)
		& MQUEUE_SLOT_STATUS_OCCUPIED_MASK
		>> MQUEUE_SLOT_STATUS_OCCUPIED_SHIFT;
	if (!freeslot)
		return -EAGAIN;

	/* Get the slot in order to write into it */
	fmc_writel(fmc, MQUEUE_CMD_CLAIM, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	*seq = ++trtl->message_sequence;
	/* Assign a sequence number to the message */
	data[1] = *seq;
	/* Write data into the slot */
	for (i = 0; i < size / 4; ++i) {
		fmc_writel(fmc, data[i],
			   hmq->base_sr + MQUEUE_SLOT_DATA_START + i * 4);
		dev_vdbg(&hmq->dev, "From HOST to MT data[%i] = 0x%08x\n",
		  i, data[i]);
	}

	/* The slot is ready to be sent to the CPU */
	fmc_writel(fmc, MQUEUE_CMD_READY | ((size / 4) & 0xFF),
		   hmq->base_sr + MQUEUE_SLOT_COMMAND);

	hmq->stats.count++;

	return 0;
}


/**
 * Send a message and wait for the answer
 */
static int trtl_ioctl_msg_sync(struct trtl_hmq *hmq, void __user *uarg)
{
	struct trtl_dev *trtl = to_trtl_dev(hmq->dev.parent);
	struct trtl_msg msg_ans, msg_req;
	struct trtl_msg_sync msg;
	struct trtl_hmq *hmq_out;
	unsigned long flags;
	int err = 0, to;

	/* Copy the message from user space*/
	err = copy_from_user(&msg, uarg, sizeof(struct trtl_msg_sync));
	if (err)
		return err;
	err = copy_from_user(&msg_req, msg.msg, sizeof(struct trtl_msg));
	if (err)
		return err;

	if (hmq->index != msg.index_in) {
		dev_warn(&hmq->dev,
			 "cannot enqueue messages on other slots\n");
		return -EINVAL;
	}
	if (msg.index_out >= trtl->n_hmq_out) {
		dev_err(&hmq->dev, "un-existent slot %d\n", msg.index_out);
		return -EINVAL;
	}
	if (msg_req.datalen * 4 >= hmq->buf.max_msg_size) {
		dev_err(&hmq->dev,
			"Cannot send %d bytes, the maximum size is %d bytes\n",
			msg_req.datalen * 4, hmq->max_width * 4);
		return -EINVAL;
	}
	hmq_out = &trtl->hmq_out[msg.index_out];

	/* Use mutex to serialize sync messages. */
	mutex_lock(&hmq->mtx_sync);
	mutex_lock(&hmq_out->mtx_sync);

	/* Rise wait flag */
	spin_lock_irqsave(&hmq_out->lock, flags);
	hmq_out->flags |= TRTL_FLAG_HMQ_SYNC_WAIT;
	spin_unlock_irqrestore(&hmq_out->lock, flags);

	/* Send the message */
	spin_lock_irqsave(&hmq->lock, flags);
	err = trtl_message_push(hmq, msg_req.data,
				msg_req.datalen * 4, &hmq_out->waiting_seq);
	spin_unlock_irqrestore(&hmq->lock, flags);
	if (err)
		return err;

	/*
	 * Wait our synchronous answer. If after timeout we don't receive
	 * an answer, something is seriously broken
	 */
	msg.timeout_ms = msg.timeout_ms ? msg.timeout_ms : hmq_sync_timeout;
	to = wait_event_interruptible_timeout(hmq_out->q_msg,
					      hmq_out->flags & TRTL_FLAG_HMQ_SYNC_READY,
					      msecs_to_jiffies(msg.timeout_ms));

	spin_lock_irqsave(&hmq_out->lock, flags);
	hmq_out->flags &= ~TRTL_FLAG_HMQ_SYNC_READY;
	msg_ans = hmq_out->sync_answer;
	spin_unlock_irqrestore(&hmq_out->lock, flags);

	mutex_unlock(&hmq_out->mtx_sync);
	mutex_unlock(&hmq->mtx_sync);

	/* On error, or timeout, clear the message.
	 * This should not happen, so optimize
	 */
	if (unlikely(to <= 0))
		memset(&msg_ans, 0, sizeof(struct trtl_msg));

	/* On timeout print an error message.
	 * This should not happen, so optimize
	 */
	if (unlikely(to == 0))
		dev_err(&hmq->dev,
			"The real time application is taking too much time to answer (more than %dms). Something is broken\n",
			msg.timeout_ms);

	/* Return the error code on error, or update with remaining time
	 * Errors should not happen, so optimize
	 */
	if (unlikely(to < 0))
		return to;
	else
		msg.timeout_ms = jiffies_to_msecs(to);

	err = copy_to_user(msg.msg, &msg_ans, sizeof(struct trtl_msg));
	if (err)
		return err;
	return copy_to_user(uarg, &msg, sizeof(struct trtl_msg_sync));
}


/**
 * FIXME: to be tested
 * Add a filter rule to a given file-descriptor
 */
static int trtl_ioctl_msg_filter_add(struct trtl_hmq_user *user,
				     void __user *uarg)
{
	struct trtl_msg_filter_element *fltel;
	struct trtl_msg_filter u_filter;
	int err = 0;

	/* Copy the message from user space*/
	err = copy_from_user(&u_filter, uarg, sizeof(struct trtl_msg_filter));
	if (err)
		return err;

	fltel = kmalloc(sizeof(struct trtl_msg_filter_element), GFP_KERNEL);
	if (!fltel)
		return -ENOMEM;

	/* Copy the filter */
	memcpy(&fltel->filter, &u_filter, sizeof(struct trtl_msg_filter));

	/* TODO validate filter - word_offset less that max_msg_size */

	/* Store filter */
	spin_lock(&user->lock_filter);
	list_add_tail(&fltel->list, &user->list_filters);
	user->n_filters++;
	spin_unlock(&user->lock_filter);

	return 0;
}


/**
 * FIXME: to be tested
 * Remove all filter rules form a given file-descriptor
 */
static void trtl_ioctl_msg_filter_clean(struct trtl_hmq_user *user,
				       void __user *uarg)
{
	struct trtl_msg_filter_element *fltel, *tmp;

	spin_lock(&user->lock_filter);
	list_for_each_entry_safe (fltel, tmp, &user->list_filters, list) {
		list_del(&fltel->list);
		kfree(fltel);
		user->n_filters--;
	}
	spin_unlock(&user->lock_filter);
}


/**
 * Set of special operations that can be done on the HMQ
 */
static long trtl_hmq_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct trtl_hmq_user *user = f->private_data;
	struct trtl_hmq *hmq = user->hmq;
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
	case TRTL_IOCTL_MSG_SYNC:
		err = trtl_ioctl_msg_sync(hmq, uarg);
		break;
	case TRTL_IOCTL_MSG_FILTER_ADD:
		err = trtl_ioctl_msg_filter_add(user, uarg);
		break;
	case TRTL_MSG_FILTER_CLEAN:
		trtl_ioctl_msg_filter_clean(user, uarg);
		break;
	default:
		pr_warn("trtl: invalid ioctl command %d\n", cmd);
		return -EINVAL;
	}

	return err;
}


/**
 * It returns a message to user space messages from an output HMQ
 */
static ssize_t trtl_hmq_read(struct file *f, char __user *buf,
			     size_t count, loff_t *offp)
{
	struct trtl_hmq_user *user = f->private_data;
	struct trtl_hmq *hmq = user->hmq;
	struct trtl_msg msg;
	unsigned int i = 0, n;
	int err = 0;

	if (hmq->flags & TRTL_FLAG_HMQ_DIR) {
		dev_err(&hmq->dev, "cannot read from an input queue\n");
		return -EFAULT;
	}

	/* Calculate the number of messages to read */
	if (count % sizeof(struct trtl_msg)) {
		dev_err(&hmq->dev,
			"we can read only entire messages (single message size %zu, requested size %zu)\n",
			sizeof(struct trtl_msg), count);
		return -EINVAL;
	}
	n = count / sizeof(struct trtl_msg);

	count = 0;
	mutex_lock(&hmq->mtx); /* Not really useful mutex for the time being */
	/* read as much as we can */
	while (!err && i < n && CIRC_CNT(hmq->buf.ptr_w, user->ptr_r, hmq->buf.size)) {
		spin_lock(&hmq->lock);
		if (!trtl_hmq_filter_check(user, hmq->buf.mem + user->ptr_r)) {
			/* The current message is of no interest for the user */
			user->ptr_r += hmq->buf.max_msg_size;
			if (user->ptr_r >= hmq->buf.size) {
				user->ptr_r = 0;
			}
			spin_unlock(&hmq->lock);
			continue;
		}

		/* Copy to user space buffer - not optimal because inherited
		   mechanism that I cannot change it now */
		memcpy(msg.data, hmq->buf.mem + user->ptr_r, hmq->buf.max_msg_size);
		msg.datalen = hmq->buf.max_msg_size / 4;
		if (copy_to_user(buf + count, &msg, sizeof(struct trtl_msg))) {
			dev_err(&hmq->dev, "Cannot message transfer to user-space\n");
			err = -EFAULT;
			spin_unlock(&hmq->lock);
			break;
		}

		count = (++i) * sizeof(struct trtl_msg);
		/* Point to the next message */
		user->ptr_r += hmq->buf.max_msg_size;
		if (user->ptr_r >= hmq->buf.size) {
			user->ptr_r = 0;
		}
		spin_unlock(&hmq->lock);
	}
	mutex_unlock(&hmq->mtx);

	*offp += count;
	return count ? count : err;
}

/**
 * For HMQ output it checks if there is something in our message queue to read
 * For HMQ input it checks if there is space in our message queue to write
 */
static unsigned int trtl_hmq_poll(struct file *f, struct poll_table_struct *w)
{
	struct trtl_hmq_user *user = f->private_data;
	struct trtl_hmq *hmq = user->hmq;
	unsigned int ret = 0;

	poll_wait(f, &hmq->q_msg, w);

	if (hmq->flags & TRTL_FLAG_HMQ_DIR) { /* MockTurtle input */
		/* Check if we have free space */
		if (CIRC_SPACE(hmq->buf.ptr_w, hmq->buf.ptr_r, hmq->buf.size))
			ret |= POLLOUT | POLLWRNORM;
	} else { /* MockTurtle output */
		/* Check if we have something to read */
		if (CIRC_CNT(hmq->buf.ptr_w, user->ptr_r, hmq->buf.size))
			ret |= POLLIN | POLLRDNORM;
	}

	return ret;
}

const struct file_operations trtl_hmq_fops = {
	.owner = THIS_MODULE,
	.open  = trtl_hmq_open,
	.release = trtl_hmq_release,
	.write  = trtl_hmq_write,
	.read = trtl_hmq_read,
	.unlocked_ioctl = trtl_hmq_ioctl,
	.poll = trtl_hmq_poll,
};


/**
 * It handles an input interrupts. The CPU is waiting for input data, so
 * we should feed the CPU if we have something in our local buffer.
 *
 * @TODO to be tested. For the time being we are working only in
 *       synchronous mode
 */
static void trtl_irq_handler_input(struct trtl_hmq *hmq)
{
	struct trtl_dev *trtl = to_trtl_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	struct trtl_msg_element *msgel;
	unsigned long flags;
	uint32_t mask, seq;
	int err;

	/*
	 * If the mutex is locked, then someone is writing on the message list.
	 * In order to avoid to send wrong messages (read 'write' function
	 * implementation and comments), do not send anything to the CPU input
	 * until the all write processes are over.
	 */
	if (mutex_is_locked(&hmq->mtx))
		return;

	if (CIRC_CNT(hmq->buf.ptr_w, hmq->buf.ptr_r, hmq->buf.size) == 0) {
		/*
		 * We don't have nothing to send, disable the CPU input ready
		 * interrupts
		 */
		spin_lock_irqsave(&hmq->lock, flags);
		mask = fmc_readl(fmc, trtl->base_gcr + MQUEUE_GCR_IRQ_MASK);
		mask &= ~(1 << (hmq->index + MQUEUE_GCR_IRQ_MASK_IN_SHIFT));
		fmc_writel(fmc, mask, trtl->base_gcr + MQUEUE_GCR_IRQ_MASK);
		spin_unlock_irqrestore(&hmq->lock, flags);

		/* Wake up processes waiting for this */
		wake_up_interruptible(&hmq->q_msg);
		return;
	}

	spin_lock_irqsave(&hmq->lock, flags);
	err = trtl_message_push(hmq, msgel->msg,
				hmq->buf.max_msg_size * 4, &seq);  /* we don't care about seq num */
	if (err) {
		dev_err(&hmq->dev,
			"Cannot send message %d\n", seq);
	}
	hmq->buf.ptr_r = (hmq->buf.ptr_r + hmq->buf.max_msg_size) & (hmq->buf.size - 1);

	spin_unlock_irqrestore(&hmq->lock, flags);
}

/**
 * It handles an output interrupt. It means that the CPU is outputting
 * data for us, so we must read it.
 */
static void trtl_irq_handler_output(struct trtl_hmq *hmq)
{
	struct trtl_dev *trtl = to_trtl_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	struct mturtle_hmq_buffer *buf = &hmq->buf;
	uint32_t status, *buffer = buf->mem + buf->ptr_w;
	size_t size;
	int i, left_byte;
	struct trtl_hmq_user *usr, *tmp;
	unsigned long flags;

	spin_lock_irqsave(&hmq->lock, flags);
	/* Get the message */
	/* Get information about the incoming slot */
	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);
	size = (status & MQUEUE_SLOT_STATUS_MSG_SIZE_MASK);
	size >>= MQUEUE_SLOT_STATUS_MSG_SIZE_SHIFT;
	/* Read data from the slot */
	for (i = 0; i < size; ++i) {
		buffer[i] = fmc_readl(fmc,
				hmq->base_sr + MQUEUE_SLOT_DATA_START + i * 4);
		/*dev_vdbg(&hmq->dev, "From MT to HOST data[%i] = 0x%08x\n",
		  i, buffer[i]);*/
	}
	/* Discard the slot content */
	fmc_writel(fmc, MQUEUE_CMD_DISCARD, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	spin_unlock_irqrestore(&hmq->lock, flags);

	hmq->stats.count++;

	/* If we are waiting a synchronous answer on this HMQ check */
	if ((hmq->flags & TRTL_FLAG_HMQ_SYNC_WAIT) &&
	    hmq->waiting_seq == buffer[1]) { /* seq number always position 1 */
		spin_lock_irqsave(&hmq->lock, flags);
		memcpy(hmq->sync_answer.data, buffer, size * 4);
		hmq->sync_answer.datalen = size;
		hmq->flags &= ~TRTL_FLAG_HMQ_SYNC_WAIT;
		hmq->flags |= TRTL_FLAG_HMQ_SYNC_READY;
		spin_unlock_irqrestore(&hmq->lock, flags);

		/* Do not store synchronous answer */
		goto out;
	}


	/*
	 * Update user pointer when the write pointer is overwriting data
	 * not yet read by the user. Loop over all users and check their
	 * read-pointer
	 */
	list_for_each_entry_safe(usr, tmp, &hmq->list_usr, list) {
		/* TODO check user read pointer if overwritten */
		spin_lock_irqsave(&usr->lock, flags);
		left_byte = CIRC_SPACE(buf->ptr_w, usr->ptr_r, buf->size);
		if (left_byte <= buf->max_msg_size)
			usr->ptr_r = (usr->ptr_r + buf->max_msg_size) & (hmq->buf.size - 1);
		spin_unlock_irqrestore(&usr->lock, flags);
	}

	/* Update write pointer for the next pop */
	spin_lock_irqsave(&hmq->lock, flags);
	buf->ptr_w = (buf->ptr_w + hmq->buf.max_msg_size) & (hmq->buf.size - 1);
	spin_unlock_irqrestore(&hmq->lock, flags);

 out:
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
irqreturn_t trtl_irq_handler(int irq_core_base, void *arg)
{
	struct fmc_device *fmc = arg;
	struct trtl_dev *trtl = fmc_get_drvdata(fmc);
	uint32_t status;
	int i, j, n_disp = 0;

	/* Get the source of interrupt */
	status = fmc_readl(fmc, trtl->base_gcr + MQUEUE_GCR_SLOT_STATUS);
	if (!status)
		return IRQ_NONE;
	status &= trtl->irq_mask;
 dispatch_irq:
	n_disp++;
	i = -1;
	while (status && i < TRTL_MAX_HMQ_SLOT) {
		++i;
		if (!(status & 0x1)) {
			status >>= 1;
			continue;
		}

		if (i >= MAX_MQUEUE_SLOTS) {
			j = i - MAX_MQUEUE_SLOTS;
			trtl_irq_handler_input(&trtl->hmq_in[j]);
		} else {
			trtl_irq_handler_output(&trtl->hmq_out[i]);
		}
		/* Clear handled interrupts */
		status >>= 1;
	}
	/*
	 * check if other interrupts occurs in the meanwhile
	 */
	status = fmc_readl(fmc, trtl->base_gcr + MQUEUE_GCR_SLOT_STATUS);
	status &= trtl->irq_mask;
	if (status && n_disp < hmq_max_irq_loop)
		goto dispatch_irq;

	fmc_irq_ack(fmc);

	return IRQ_HANDLED;
}
