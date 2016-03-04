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

#include <linux/fmc.h>


#include <hw/mqueue.h>

#include "wrnc.h"

int hmq_default_buf_size = 8192; /**< default buffer size in byte */
module_param_named(slot_buffer_size, hmq_default_buf_size, int, 0444);
MODULE_PARM_DESC(slot_buffer_size, "Default buffer size in byte.");

int hmq_max_msg = 32; /**< Maximum number of messages in driver queue */
module_param_named(max_slot_msg, hmq_max_msg, int, 0444);
MODULE_PARM_DESC(max_slot_msg, "Maximum number of messages in driver queue.");

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

static int wrnc_message_push(struct wrnc_hmq *hmq, struct wrnc_msg *msg,
			     uint32_t *seq);

/**
 * It applies filters on a given message.
 */
static int wrnc_hmq_filter_check(struct wrnc_hmq_user *user,
				 struct wrnc_msg *msg)
{
	struct wrnc_msg_filter_element *fltel, *tmp;
	unsigned int passed = 1;
	uint32_t word;

	spin_lock(&user->lock_filter);
	list_for_each_entry_safe (fltel, tmp, &user->list_filters, list) {
		/* If one of the previous filter failed, then stop */
		if (!passed)
			break;

		word = msg->data[fltel->filter.word_offset];
		switch(fltel->filter.operation) {
		case WRNC_MSG_FILTER_AND:
			word &= fltel->filter.mask;
			break;
		case WRNC_MSG_FILTER_OR:
			word |= fltel->filter.mask;
			break;
		case WRNC_MSG_FILTER_EQ:
			break;
#if 0 /* FIXME not clear from specification what NOT should do*/
		case WRNC_MSG_FILTER_NOT:
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
 * Dispatch messages to all listeners. This allow multiple readers to get the
 * messages
 */
static void wrnc_hmq_dispatch_out(struct wrnc_hmq *hmq,
				  struct wrnc_msg_element *msgel)
{
	struct wrnc_msg_element *new;
	struct wrnc_msg *msg;
	struct wrnc_hmq_user *usr, *tmp;
	unsigned long flags;

	/* If we are waiting a synchronous answer on this HMQ check */
	if ((hmq->flags & WRNC_FLAG_HMQ_SYNC_WAIT) &&
	    hmq->waiting_seq == wrnc_get_sequence(msgel->msg)) {
		spin_lock_irqsave(&hmq->lock, flags);
		hmq->sync_answer = *msgel->msg;
		hmq->flags &= ~WRNC_FLAG_HMQ_SYNC_WAIT;
		hmq->flags |= WRNC_FLAG_HMQ_SYNC_READY;
		spin_unlock_irqrestore(&hmq->lock, flags);

		/* Do not store synchronous answer */
		return;
	}

	/* for each user list copy the message */
	list_for_each_entry_safe(usr, tmp, &hmq->list_usr, list) {
		/* Filter the message */
		if (!wrnc_hmq_filter_check(usr, msgel->msg))
			continue;

		/* Create a copy */
		new = kmalloc(sizeof(struct wrnc_msg_element), GFP_ATOMIC);
		if (!new)
			continue;
		msg = kmalloc(sizeof(struct wrnc_msg), GFP_ATOMIC);
		if (!msg) {
			kfree(new);
			continue;
		}
		memcpy(msg, msgel->msg, sizeof(struct wrnc_msg));
		new->msg = msg;


		/* Save the copy within user's list */
		spin_lock_irqsave(&usr->lock, flags);
		list_add_tail(&new->list, &usr->list_msg_output);
	        usr->n_output++;
		spin_unlock_irqrestore(&usr->lock, flags);

		if (unlikely(usr->n_output > hmq->max_msg)) {
			/* there is no more space, remove the oldest message */
			spin_lock_irqsave(&usr->lock, flags);
		        new = list_entry(usr->list_msg_output.next,
					   struct wrnc_msg_element, list);
			list_del(&new->list);
		        usr->n_output--;
			spin_unlock_irqrestore(&usr->lock, flags);

			kfree(new->msg);
			kfree(new);
		}
	}
}


/**
 * It return 1 if the message quque slot is full
 */
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


/**
 * It return 1 if the message quque slot is empty
 */
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


/**
 * It returns the number of messages in the WRNC queue
 */
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


/**
 * It returns the maximum number of messages in the WRNC queue
 */
static ssize_t wrnc_show_count_max_sw(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);

	return sprintf(buf, "%d\n", hmq->max_msg);
}

static ssize_t wrnc_store_count_max_sw(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;
	if (val < 1) {
		dev_err(dev,
			"Buffer length must be greater or equal 1, got %ld\n",
			val);
		return -EINVAL;
	}

	hmq->max_msg = val;

	return count;
}


/**
 * It returns the maximum number of messages in the WRNC queue
 */
static ssize_t wrnc_show_count_max_hw(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);

	return sprintf(buf, "%d\n", hmq->max_depth);
}


/**
 * It returns the maximum number bytes per message
 */
static ssize_t wrnc_show_width_max(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);

	return sprintf(buf, "%d\n", hmq->max_width * 4);
}

/**
 * Show the current share status of the HMQ slot
 */
static ssize_t wrnc_show_share(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);

	return sprintf(buf, "%d\n", !!(hmq->flags & WRNC_FLAG_HMQ_SHR_USR));
}


/**
 * Set if the char-device are in shared mode or not
 */
static ssize_t wrnc_store_share(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct wrnc_hmq *hmq = to_wrnc_hmq(dev);
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	/*
	 * If the status is the same, than there is nothing to do. This
	 * control sounds useless but it save code in user-space and it
	 * allows a proper error management
	 */
	if (val == (!!(hmq->flags & WRNC_FLAG_HMQ_SHR_USR)))
		return count;

	/* You cannot configure while the HMQ is in use */
	if (hmq->n_user > 0)
		return -EBUSY;

	spin_lock(&hmq->lock);
	if (val)
		hmq->flags |= WRNC_FLAG_HMQ_SHR_USR;
	else
		hmq->flags &= ~WRNC_FLAG_HMQ_SHR_USR;
	spin_unlock(&hmq->lock);

	return count;
}

DEVICE_ATTR(full, S_IRUGO, wrnc_show_full, NULL);
DEVICE_ATTR(empty, S_IRUGO, wrnc_show_empty, NULL);
DEVICE_ATTR(count_hw, S_IRUGO, wrnc_show_count, NULL);
DEVICE_ATTR(count_max_sw, S_IRUGO | S_IWUSR | S_IWGRP,
	    wrnc_show_count_max_sw, wrnc_store_count_max_sw);
DEVICE_ATTR(count_max_hw, S_IRUGO, wrnc_show_count_max_hw, NULL);
DEVICE_ATTR(width_max, S_IRUGO, wrnc_show_width_max, NULL);
DEVICE_ATTR(shared_by_users, (S_IRUGO | S_IWUSR | S_IWGRP |  S_IWOTH),
	    wrnc_show_share, wrnc_store_share);

static struct attribute *wrnc_hmq_attr[] = {
	&dev_attr_full.attr,
	&dev_attr_empty.attr,
	&dev_attr_count_hw.attr,
	&dev_attr_count_max_sw.attr,
	&dev_attr_count_max_hw.attr,
	&dev_attr_width_max.attr,
	&dev_attr_shared_by_users.attr,
	NULL,
};

static const struct attribute_group wrnc_hmq_group = {
	.attrs = wrnc_hmq_attr,
};

const struct attribute_group *wrnc_hmq_groups[] = {
	&wrnc_hmq_group,
	NULL,
};



/**
 * It simply opens a HMQ device
 */
static int wrnc_hmq_open(struct inode *inode, struct file *file)
{
	struct wrnc_hmq_user *user;
	struct wrnc_hmq *hmq;
	unsigned long flags;
	int m = iminor(inode);

	hmq = to_wrnc_hmq(minors[m]);

	if (list_empty(&hmq->list_usr) || (hmq->flags & WRNC_FLAG_HMQ_SHR_USR)) {
		user = kzalloc(sizeof(struct wrnc_hmq_user), GFP_KERNEL);
		if (!user)
			return -ENOMEM;

		user->hmq = hmq;
		spin_lock_init(&user->lock);
		spin_lock_init(&user->lock_filter);
		INIT_LIST_HEAD(&user->list_msg_output);
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
				   struct wrnc_hmq_user, list);
		hmq->n_user++;
		spin_unlock_irqrestore(&hmq->lock, flags);
	}

	file->private_data = user;


	return 0;
}

static int wrnc_hmq_release(struct inode *inode, struct file *f)
{
	struct wrnc_hmq_user *user = f->private_data;
	struct wrnc_hmq *hmq = user->hmq;
	struct wrnc_msg_element *msgel, *tmp;
	unsigned long flags;


	/* Remove user from the list */
	spin_lock_irqsave(&hmq->lock, flags);
	hmq->n_user--;

	if (hmq->flags & WRNC_FLAG_HMQ_SHR_USR || hmq->n_user == 0) {
		list_del(&user->list);

		/* Clean up message list */
		spin_lock(&user->lock);
		list_for_each_entry_safe(msgel, tmp, &user->list_msg_output, list) {
			list_del(&msgel->list);
			kfree(msgel);
		}
		spin_unlock_irqrestore(&user->lock, flags);

		kfree(user);
	}

	/* Reset the default shared status */
	if (hmq->n_user == 0) {
		if (hmq_shared)
			hmq->flags |= WRNC_FLAG_HMQ_SHR_USR;
		else
			hmq->flags &= ~WRNC_FLAG_HMQ_SHR_USR;
	}
	spin_unlock_irqrestore(&hmq->lock, flags);

	return 0;
}

/**
 * It writes message in the drive message queue. The messages will be sent on
 * IRQ signal
 * @TODO to be tested! WRTD is using only sync messages
 */
static ssize_t wrnc_hmq_write(struct file *f, const char __user *buf,
			      size_t count, loff_t *offp)
{
	struct wrnc_hmq_user *user = f->private_data;
	struct wrnc_hmq *hmq = user->hmq;
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	struct wrnc_msg_element *msgel;
	struct wrnc_msg msg;
	unsigned long flags;
	unsigned int i, n, free_slot;
	const char __user *curbuf = buf;
	uint32_t mask, seq;
	int err = 0;

	if (!(hmq->flags & WRNC_FLAG_HMQ_DIR)) {
		dev_err(&hmq->dev, "cannot write on an output queue\n");
		return -EFAULT;
	}

	if (count % sizeof(struct wrnc_msg)) {
		dev_err(&hmq->dev, "we can write only entire messages\n");
		return -EINVAL;
	}

	if (hmq->n_input >= hmq->max_msg) {
		return -EBUSY;
	}

	/* Get number of free slots */
	n = count / sizeof(struct wrnc_msg);
	free_slot = hmq->max_msg - hmq->n_input;
	n = free_slot < n ? free_slot : n;

	count = 0;
	mutex_lock(&hmq->mtx);

	for (i = 0; i < n; i++, curbuf += sizeof(struct wrnc_msg)) {
		if (copy_from_user(&msg, curbuf, sizeof(struct wrnc_msg))) {
			err = -EFAULT;
			break;
		}

		if (msg.datalen >= hmq->max_width) {
			dev_err(&hmq->dev,
				"Cannot send %d bytes, the maximum size is %d\n",
				msg.datalen * 4, hmq->max_width * 4);
			err = -EINVAL;
			break;
		}

		/* Enqueue messages if we are going to send them using
		   interrupts; otherwise sen them immediately */
		if (hmq_in_irq) {
			/* Allocate and fill message structure */
			msgel = kmalloc(sizeof(struct wrnc_msg_element), GFP_KERNEL);
			if (!msgel) {
				err = -ENOMEM;
				break;
			}

			msgel->msg = kmalloc(sizeof(struct wrnc_msg), GFP_KERNEL);
			if (!msgel->msg) {
				kfree(msgel);
				err = -ENOMEM;
				break;
			}
			memcpy(msgel->msg, &msg, sizeof(struct wrnc_msg));

			spin_lock_irqsave(&hmq->lock, flags);
			list_add_tail(&msgel->list, &hmq->list_msg_input);
			hmq->n_input++;
			spin_unlock_irqrestore(&hmq->lock, flags);
		} else {
			spin_lock_irqsave(&hmq->lock, flags);
			err = wrnc_message_push(hmq, &msg, &seq);
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
		mask = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
		mask |= (1 << (hmq->index + MQUEUE_GCR_IRQ_MASK_IN_SHIFT));
		fmc_writel(fmc, mask, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
	}

	/* Update counter */
	count = i * sizeof(struct wrnc_msg);
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
 * @param[in] msg the message to send
 * @param[out] the sequence number used to send the message
 */
static int wrnc_message_push(struct wrnc_hmq *hmq, struct wrnc_msg *msg,
			     uint32_t *seq)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t freeslot;
	int i;

	freeslot = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS)
		& MQUEUE_SLOT_STATUS_OCCUPIED_MASK
		>> MQUEUE_SLOT_STATUS_OCCUPIED_SHIFT;
	if (!freeslot)
		return -EAGAIN;

	/* Get the slot in order to write into it */
	fmc_writel(fmc, MQUEUE_CMD_CLAIM, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	*seq = ++wrnc->message_sequence;
	/* Assign a sequence number to the message */
	msg->data[1] = *seq;
	/* Write data into the slot */
	for (i = 0; i < msg->datalen && i < hmq->max_width; ++i)
		fmc_writel(fmc, msg->data[i],
			   hmq->base_sr + MQUEUE_SLOT_DATA_START + i * 4);

	/* The slot is ready to be sent to the CPU */
	fmc_writel(fmc, MQUEUE_CMD_READY | (msg->datalen & 0xFF),
		   hmq->base_sr + MQUEUE_SLOT_COMMAND);

	return 0;
}

/**
 * It reads a message from a FPGA HMQ
 */
static void wrnc_message_pop(struct wrnc_hmq *hmq, struct wrnc_msg *msg)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t status;
	int i;

	/* Get information about the incoming slot */
	status = fmc_readl(fmc, hmq->base_sr + MQUEUE_SLOT_STATUS);
	msg->datalen = (status & MQUEUE_SLOT_STATUS_MSG_SIZE_MASK);
	msg->datalen >>= MQUEUE_SLOT_STATUS_MSG_SIZE_SHIFT;
	/* Read data from the slot */
	for (i = 0; i < msg->datalen; ++i) {
		msg->data[i] = fmc_readl(fmc,
				hmq->base_sr + MQUEUE_SLOT_DATA_START + i * 4);
	}

	/* Discard the slot content */
	fmc_writel(fmc, MQUEUE_CMD_DISCARD, hmq->base_sr + MQUEUE_SLOT_COMMAND);
}


/**
 * Send a message and wait for the answer
 */
static int wrnc_ioctl_msg_sync(struct wrnc_hmq *hmq, void __user *uarg)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct wrnc_msg msg_ans, msg_req;
	struct wrnc_msg_sync msg;
	struct wrnc_hmq *hmq_out;
	unsigned long flags;
	int err = 0, to;

	/* Copy the message from user space*/
	err = copy_from_user(&msg, uarg, sizeof(struct wrnc_msg_sync));
	if (err)
		return err;
	err = copy_from_user(&msg_req, msg.msg, sizeof(struct wrnc_msg));
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
	if (msg_req.datalen >= hmq->max_width) {
		dev_err(&hmq->dev,
			"Cannot send %d bytes, the maximum size is %d\n",
			msg_req.datalen * 4, hmq->max_width * 4);
		return -EINVAL;
	}
	hmq_out = &wrnc->hmq_out[msg.index_out];

	/* Use mutex to serialize sync messages. */
	mutex_lock(&hmq->mtx_sync);
	mutex_lock(&hmq_out->mtx_sync);

	/* Rise wait flag */
	spin_lock_irqsave(&hmq_out->lock, flags);
	hmq_out->flags |= WRNC_FLAG_HMQ_SYNC_WAIT;
	spin_unlock_irqrestore(&hmq_out->lock, flags);

	/* Send the message */
	spin_lock_irqsave(&hmq->lock, flags);
	err = wrnc_message_push(hmq, &msg_req, &hmq_out->waiting_seq);
	spin_unlock_irqrestore(&hmq->lock, flags);
	if (err)
		return err;

	/*
	 * Wait our synchronous answer. If after timeout we don't receive
	 * an answer, something is seriously broken
	 */
	msg.timeout_ms = msg.timeout_ms ? msg.timeout_ms : hmq_sync_timeout;
	to = wait_event_interruptible_timeout(hmq_out->q_msg,
					      hmq_out->flags & WRNC_FLAG_HMQ_SYNC_READY,
					      msecs_to_jiffies(msg.timeout_ms));

	spin_lock_irqsave(&hmq_out->lock, flags);
	hmq_out->flags &= ~WRNC_FLAG_HMQ_SYNC_READY;
	msg_ans = hmq_out->sync_answer;
	spin_unlock_irqrestore(&hmq_out->lock, flags);

	mutex_unlock(&hmq_out->mtx_sync);
	mutex_unlock(&hmq->mtx_sync);

	/* On error, or timeout, clear the message.
	 * This should not happen, so optimize
	 */
	if (unlikely(to <= 0))
		memset(&msg_ans, 0, sizeof(struct wrnc_msg));

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

	err = copy_to_user(msg.msg, &msg_ans, sizeof(struct wrnc_msg));
	if (err)
		return err;
	return copy_to_user(uarg, &msg, sizeof(struct wrnc_msg_sync));
}


/**
 * FIXME: to be tested
 * Add a filter rule to a given file-descriptor
 */
static int wrnc_ioctl_msg_filter_add(struct wrnc_hmq_user *user,
				     void __user *uarg)
{
	struct wrnc_msg_filter_element *fltel;
	struct wrnc_msg_filter u_filter;
	int err = 0;

	/* Copy the message from user space*/
	err = copy_from_user(&u_filter, uarg, sizeof(struct wrnc_msg_filter));
	if (err)
		return err;

	fltel = kmalloc(sizeof(struct wrnc_msg_filter_element), GFP_KERNEL);
	if (!fltel)
		return -ENOMEM;

	/* Copy the filter */
	memcpy(&fltel->filter, &u_filter, sizeof(struct wrnc_msg_filter));

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
static void wrnc_ioctl_msg_filter_clean(struct wrnc_hmq_user *user,
				       void __user *uarg)
{
	struct wrnc_msg_filter_element *fltel, *tmp;

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
static long wrnc_hmq_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct wrnc_hmq_user *user = f->private_data;
	struct wrnc_hmq *hmq = user->hmq;
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
	case WRNC_IOCTL_MSG_SYNC:
		err = wrnc_ioctl_msg_sync(hmq, uarg);
		break;
	case WRNC_IOCTL_MSG_FILTER_ADD:
		err = wrnc_ioctl_msg_filter_add(user, uarg);
		break;
	case WRNC_MSG_FILTER_CLEAN:
		wrnc_ioctl_msg_filter_clean(user, uarg);
		break;
	default:
		pr_warn("wrnc: invalid ioctl command %d\n", cmd);
		return -EINVAL;
	}

	return err;
}


/**
 * It returns a message to user space messages from an output HMQ
 */
static ssize_t wrnc_hmq_read(struct file *f, char __user *buf,
			     size_t count, loff_t *offp)
{
	struct wrnc_hmq_user *user = f->private_data;
	struct wrnc_hmq *hmq = user->hmq;
	struct wrnc_msg_element *msgel;
	unsigned int i, n;
	int err = 0;

	if (hmq->flags & WRNC_FLAG_HMQ_DIR) {
		dev_err(&hmq->dev, "cannot read from an input queue\n");
		return -EFAULT;
	}

	/* Calculate the number of messages to read */
	if (count % sizeof(struct wrnc_msg)) {
		dev_err(&hmq->dev, "we can read only entire messages\n");
		return -EINVAL;
	}
	n = count / sizeof(struct wrnc_msg);


	count = 0;
	mutex_lock(&hmq->mtx); /* Not really useful mutex for the time being */
	/* read as much as we can */
	for (i = 0; i < n && !err; ++i) {
		if (list_empty(&user->list_msg_output)) {
			*offp = 0;
			break;
		}
		/* Get the oldest message in the queue */
		spin_lock(&user->lock);
		msgel = list_entry(user->list_msg_output.next,
				   struct wrnc_msg_element, list);
		list_del(&msgel->list);
	        user->n_output--;
		spin_unlock(&user->lock);

		/* Copy to user space buffer */
		if (copy_to_user(buf + count, msgel->msg,
				 sizeof(struct wrnc_msg)))
			err = -EFAULT;

		count = (i + 1) * sizeof(struct wrnc_msg);
		kfree(msgel->msg);
		kfree(msgel);
	}
	mutex_unlock(&hmq->mtx);

	*offp += count;
	return count ? count : err;
}

/**
 * For HMQ output it checks if there is something in our message queue to read
 * For HMQ input it checks if there is space in our message queue to write
 */
static unsigned int wrnc_hmq_poll(struct file *f, struct poll_table_struct *w)
{
	struct wrnc_hmq_user *user = f->private_data;
	struct wrnc_hmq *hmq = user->hmq;
	unsigned int ret = 0;

	poll_wait(f, &hmq->q_msg, w);

	if (hmq->flags & WRNC_FLAG_HMQ_DIR) { /* CPU input */
		if (hmq->n_input < hmq->max_msg)
			ret |= POLLOUT | POLLWRNORM;
	} else { /* CPU output */
		if (user->n_output)
			ret |= POLLIN | POLLRDNORM;
	}

	return ret;
}

const struct file_operations wrnc_hmq_fops = {
	.owner = THIS_MODULE,
	.open  = wrnc_hmq_open,
	.release = wrnc_hmq_release,
	.write  = wrnc_hmq_write,
	.read = wrnc_hmq_read,
	.unlocked_ioctl = wrnc_hmq_ioctl,
	.poll = wrnc_hmq_poll,
};


/**
 * It handles an input interrupts. The CPU is waiting for input data, so
 * we should feed the CPU if we have something in our local buffer.
 *
 * @TODO to be tested. For the time being we are working only in
 *       synchronous mode
 */
static void wrnc_irq_handler_input(struct wrnc_hmq *hmq)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	struct wrnc_msg_element *msgel;
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

	spin_lock_irqsave(&hmq->lock, flags);
	if (list_empty(&hmq->list_msg_input)) {
		/*
		 * We don't have nothing to send, disable the CPU input ready
s		 * interrupts
		 */
		mask = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
		mask &= ~(1 << (hmq->index + MQUEUE_GCR_IRQ_MASK_IN_SHIFT));
		fmc_writel(fmc, mask, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
		spin_unlock_irqrestore(&hmq->lock, flags);

		/* Wake up processes waiting for this */
		wake_up_interruptible(&hmq->q_msg);
		return;
	}

	/* Retrieve and send the first message */
	msgel = list_entry(hmq->list_msg_input.next, struct wrnc_msg_element, list);
	list_del(&msgel->list);
	hmq->n_input--;
	err = wrnc_message_push(hmq, msgel->msg, &seq);  /* we don't care about seq num */
	if (err) {
		dev_err(&hmq->dev,
			"Cannot send message %d\n", seq);
	}
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
	struct wrnc_msg_element msgel;
	struct wrnc_msg msg;
	unsigned long flags;

	/* get the message from the device */
	spin_lock_irqsave(&hmq->lock, flags);
	wrnc_message_pop(hmq, &msg);
	msgel.msg = &msg;
	spin_unlock_irqrestore(&hmq->lock, flags);

	/* Dispatch to all users or to the one who is waiting a sync message */
	wrnc_hmq_dispatch_out(hmq, &msgel);

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
	int i, j, n_disp = 0;

	/* Get the source of interrupt */
	status = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_SLOT_STATUS);
	if (!status)
		return IRQ_NONE;
	status &= wrnc->irq_mask;
 dispatch_irq:
	n_disp++;
	i = -1;
	while (status && i < WRNC_MAX_HMQ_SLOT) {
		++i;
		if (!(status & 0x1)) {
			status >>= 1;
			continue;
		}

		if (i >= MAX_MQUEUE_SLOTS) {
			j = i - MAX_MQUEUE_SLOTS;
			wrnc_irq_handler_input(&wrnc->hmq_in[j]);
		} else {
			wrnc_irq_handler_output(&wrnc->hmq_out[i]);
		}
		/* Clear handled interrupts */
		status >>= 1;
	}
	/*
	 * check if other interrupts occurs in the meanwhile
	 */
	status = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_SLOT_STATUS);
	status &= wrnc->irq_mask;
	if (status && n_disp < hmq_max_irq_loop)
		goto dispatch_irq;

	fmc_irq_ack(fmc);

	return IRQ_HANDLED;
}
