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

#include <linux/fmc.h>


#include <hw/mqueue.h>

#include "wrnc.h"

int hmq_max_msg = 32; /**< Maximum number of messages in driver queue */
module_param_named(max_slot_msg, hmq_max_msg, int, 0444);
MODULE_PARM_DESC(max_slot_msg, "Maximum number of messages in driver queue.");

int hmq_max_con = 8; /**< Maximum number connection for each slot */
module_param_named(max_slot_con, hmq_max_con, int, 0444);
MODULE_PARM_DESC(max_slot_con, "Maximum number connection for each slot.");

int hmq_sync_timeout = 1000; /**< Milli-seconds to wait for a synchronous answer */
module_param_named(sync_timeout, hmq_sync_timeout, int, 0444);
MODULE_PARM_DESC(sync_timeout, "Milli-seconds to wait for a synchronous answer.");


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

const struct attribute_group *wrnc_hmq_groups[] = {
	&wrnc_hmq_group,
	NULL,
};



/**
 * It simply opens a HMQ device
 */
static int wrnc_hmq_simple_open(struct inode *inode, struct file *file)
{
	int m = iminor(inode);

	file->private_data = to_wrnc_hmq(minors[m]);

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
	struct wrnc_hmq *hmq = f->private_data;
	struct wrnc_msg_element *msgel;
	unsigned int i, n, free_slot;
	char *curbuf = buf;
	int err = 0;

	if (!(hmq->flags & WRNC_FLAG_HMQ_DIR)) {
		dev_err(&hmq->dev, "cannot write on an output queue\n");
		return -EFAULT;
	}

	if (count % sizeof(struct wrnc_msg)) {
		dev_err(&hmq->dev, "we can write only entire messages\n");
		return -EINVAL;
	}

	if (hmq->count >= hmq_max_msg) {
		return -EBUSY;
	}

	/* Get number of free slots */
	n = count / sizeof(struct wrnc_msg);
	free_slot = hmq_max_msg - hmq->count;
	n = free_slot < n ? free_slot : n;

	/* Write all the messages */
	count = 0;
	mutex_lock(&hmq->mtx);
	for (i = 0; i < n; i++, curbuf += sizeof(struct wrnc_msg)) {
		/* Allocate and fill message structure */
		msgel = kmalloc(sizeof(struct wrnc_msg_element), GFP_KERNEL);
		if (msgel) {
			err = -ENOMEM;
			break;
		}

		msgel->msg = kmalloc(sizeof(struct wrnc_msg), GFP_KERNEL);
		if (!msgel->msg) {
			kfree(msgel);
			err = -ENOMEM;
			break;
		}

		if (copy_from_user(msgel->msg, curbuf, sizeof(struct wrnc_msg))) {
			kfree(msgel->msg);
			kfree(msgel);
			err = -EFAULT;
			break;
		}

		spin_lock(&hmq->lock);
		list_add_tail(&msgel->list, &hmq->list_msg);
		hmq->count++;
		spin_unlock(&hmq->lock);
	}
	mutex_unlock(&hmq->mtx);

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
 * It writes a message to a FPGA HMQ
 */
static uint32_t wrnc_message_push(struct wrnc_hmq *hmq, struct wrnc_msg *msg)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	unsigned long flags;
	uint32_t seq;
	int i;

	spin_lock_irqsave(&hmq->lock, flags);
	/* Get the slot in order to write into it */
	fmc_writel(fmc, MQUEUE_CMD_CLAIM, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	seq = ++wrnc->message_sequence;
	/* Assign a sequence number to the message */
	msg->data[1] = seq;
	/* Write data into the slot */
	for (i = 0; i < msg->datalen; ++i) {
		fmc_writel(fmc, msg->data[i],
			   hmq->base_sr + MQUEUE_SLOT_DATA_START + i * 4);
	}
	/* The slot is ready to be sent to the CPU */
	fmc_writel(fmc, MQUEUE_CMD_READY, hmq->base_sr + MQUEUE_SLOT_COMMAND);
	spin_unlock_irqrestore(&hmq->lock, flags);

	return seq;
}

/**
 * It reads a message from a FPGA HMQ
 */
static struct wrnc_msg *wrnc_message_pop(struct wrnc_hmq *hmq)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	struct wrnc_msg *msg;
	unsigned long flags;
	uint32_t status;
	int i;

	msg = kmalloc(sizeof(struct wrnc_msg), GFP_ATOMIC);
	if (!msg)
		return ERR_PTR(-ENOMEM);

	spin_lock_irqsave(&hmq->lock, flags);
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
	spin_unlock_irqrestore(&hmq->lock, flags);

	return msg;
}


/**
 * Look for a particular message
 */
static struct wrnc_msg_element *wrnc_retr_message(struct wrnc_hmq *hmq,
						  uint32_t sequence)
{
	struct wrnc_msg_element *tmp, *msg_out;
	int found = 0;

	spin_lock(&hmq->lock);
	list_for_each_entry_safe(msg_out, tmp, &hmq->list_msg, list) {
		if (msg_out->msg->data[1] == sequence) {
			/* We found the message, return it */
			list_del(&msg_out->list);
			hmq->count--;
			found = 1;  /* found */
			break;
		}
	}
	spin_unlock(&hmq->lock);

	return found ? msg_out : 0;  /* not found */
}

/**
 * Send a message and wait for the answer
 */
static int wrnc_ioctl_msg_sync(struct wrnc_hmq *hmq, void __user *uarg)
{
	struct wrnc_msg_element *msgel = NULL;
	struct wrnc_dev *wrnc = to_wrnc_dev(hmq->dev.parent);
	struct wrnc_msg_sync msg;
	struct wrnc_hmq *hmq_out;
	uint32_t seq;
	int err = 0, to;

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

	/* Send the message */
	seq = wrnc_message_push(hmq, &msg.msg);

	/*
	 * Wait our synchronous answer. If after 1000ms we don't receive
	 * an answer, something is seriously broken
	 */
	to = wait_event_interruptible_timeout(hmq_out->q_msg,
				 (msgel = wrnc_retr_message(hmq_out, seq)),
				 msecs_to_jiffies(hmq_sync_timeout));
	if (unlikely(to <= 0)) {
		if (to == 0)
			dev_err(&hmq->dev,
				 "The real time application is taking too much time to answer. Something is broken\n");
		memset(&msg.msg, 0, sizeof(struct wrnc_msg));
		goto out_sync;
	}

	if (!msgel)
		return -EINVAL;
	/* Copy the answer message back to user space */
	memcpy(&msg.msg, msgel->msg, sizeof(struct wrnc_msg));
	kfree(msgel->msg);
	kfree(msgel);

out_sync:
	return copy_to_user(uarg, &msg, sizeof(struct wrnc_msg_sync));
}

/**
 * Set of special operations that can be done on the HMQ
 */
static long wrnc_hmq_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct wrnc_hmq *hmq = f->private_data;
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
	default:
		pr_warn("ual: invalid ioctl command %d\n", cmd);
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
	struct wrnc_hmq *hmq = f->private_data;
	struct wrnc_msg_element *msgel;
	unsigned int i, n;

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
	/* read as much as we can */
	for (i = 0; i < n; ++i) {
		if (list_empty(&hmq->list_msg)) {
			*offp = 0;
			break;
		}
		/* Get the oldest message in the queue */
		spin_lock(&hmq->lock);
		msgel = list_entry(hmq->list_msg.next,
				   struct wrnc_msg_element, list);
		list_del(&msgel->list);
		hmq->count--;
		spin_unlock(&hmq->lock);

		/* Copy to user space buffer */
		if (copy_to_user(buf + count, msgel->msg,
				 sizeof(struct wrnc_msg)))
			return -EFAULT;

		count = (i + 1) * sizeof(struct wrnc_msg);
		kfree(msgel->msg);
		kfree(msgel);
	}

	*offp += count;

	return count;
}

/**
 * For HMQ output it checks if there is something in our message queue to read
 * For HMQ input it checks if there is space in our message queue to write
 */
static unsigned int wrnc_hmq_poll(struct file *f, struct poll_table_struct *w)
{
	struct wrnc_hmq *hmq = f->private_data;
	unsigned int ret = 0;

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

const struct file_operations wrnc_hmq_fops = {
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
	uint32_t mask;

	spin_lock_irqsave(&hmq->lock, flags);
	if (list_empty(&hmq->list_msg)) {
		/* We don't have nothing to send, disable the interrupts */
		mask = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
		mask &= ~((1 << hmq->index) + MAX_MQUEUE_SLOTS);
		fmc_writel(fmc, mask, wrnc->base_gcr + MQUEUE_GCR_IRQ_MASK);
		spin_unlock_irqrestore(&hmq->lock, flags);

		/* Wake up processes waiting for this */
		wake_up_interruptible(&hmq->q_msg);
		return;
	}

	/* Retrieve and send the first message */
	msgel = list_entry(hmq->list_msg.next, struct wrnc_msg_element, list);
	list_del(&msgel->list);
	hmq->count--;
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

	/* Allocate space for the incoming message */
	msgel = kmalloc(sizeof(struct wrnc_msg_element), GFP_KERNEL);
	if (!msgel)
		return;

	/* get the message from the device */
	msgel->msg = wrnc_message_pop(hmq);
	if (IS_ERR_OR_NULL(msgel->msg)) {
		kfree(msgel);
		return;
	}

	/* We have a valid message, store it */
	spin_lock_irqsave(&hmq->lock, flags);
	list_add_tail(&msgel->list, &hmq->list_msg);
	hmq->count++;
	if (unlikely(hmq->count > hmq_max_msg)) {
		/* there is no more space, remove the oldest message */
		msgel = list_entry(hmq->list_msg.next,
				   struct wrnc_msg_element, list);
		list_del(&msgel->list);
		hmq->count--;
		kfree(msgel->msg);
		kfree(msgel);
	}
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
	int i, j;

	/* Get the source of interrupt */
	status = fmc_readl(fmc, wrnc->base_gcr + MQUEUE_GCR_SLOT_STATUS);
	status &= wrnc->irq_mask;
dispatch_irq:
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
	if (status)
		goto dispatch_irq;

	fmc->op->irq_ack(fmc);

	return IRQ_HANDLED;
}
