/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/circ_buf.h>

#include <linux/fmc.h>
#include <hw/mockturtle_cpu_csr.h>

#include "mockturtle-drv.h"

int dbg_max_msg = 1024; /**< debug messages buffer */
module_param_named(max_dbg_msg, dbg_max_msg, int, 0444);
MODULE_PARM_DESC(max_dbg_msg, "Maximum number of debug messages in driver queue.");


static int trtl_dbg_open(struct inode *inode, struct file *file)
{
	struct trtl_cpu *cpu;

	if (inode->i_private)
		file->private_data = inode->i_private;
	cpu = file->private_data;

	cpu->cbuf.buf = kmalloc(dbg_max_msg, GFP_KERNEL);
	if (!cpu->cbuf.buf)
		if (dbg_max_msg > 0)
			return -ENOMEM;

	/* when dbg_max_msg is 0 we want to keep the debug interface
	   available so that programs will not complain */

	cpu->cbuf.head = 0;
	cpu->cbuf.tail = 0;

	return 0;
}

static int trtl_dbg_close(struct inode *inode, struct file *file)
{
	struct trtl_cpu *cpu = file->private_data;

	spin_lock(&cpu->lock);
	kfree(cpu->cbuf.buf);
	cpu->cbuf.buf = NULL;
	spin_unlock(&cpu->lock);

	return 0;
}

static ssize_t trtl_dbg_read(struct file *f, char __user *buf,
			     size_t count, loff_t *offp)
{
	struct trtl_cpu *cpu = f->private_data;
	size_t lcount;
	int i;

	spin_lock(&cpu->lock);

	/* Check if there are char to send */
	lcount = CIRC_CNT(cpu->cbuf.head, cpu->cbuf.tail, dbg_max_msg);
	if (!lcount) {
		spin_unlock(&cpu->lock);
		return 0;
	}

	/* Copy to user the minumum number of char */
	lcount = min(lcount, count);
	for (i = 0; i < lcount; i++) {
		if (cpu->cbuf.buf[cpu->cbuf.tail + i] == '\0') {
			i++; /* We want to include the terminator */
			break;
		}
	}

	lcount = i;
	if (copy_to_user(buf, cpu->cbuf.buf + cpu->cbuf.tail, lcount)) {
		spin_unlock(&cpu->lock);
		return -EFAULT;
	}

	/* Consume char from the tail */
	cpu->cbuf.tail = (cpu->cbuf.tail + lcount) & (dbg_max_msg - 1);
	spin_unlock(&cpu->lock);

	return lcount;
}

static unsigned int trtl_dbg_poll(struct file *f, struct poll_table_struct *w)
{
	struct trtl_cpu *cpu = f->private_data;

	dev_dbg(&cpu->dev, "%s  head=%d, tail=%d\n", __func__,
		cpu->cbuf.head, cpu->cbuf.tail);
	if (CIRC_CNT(cpu->cbuf.head, cpu->cbuf.tail, dbg_max_msg))
		return POLLIN | POLLRDNORM;
	return 0;
}

const struct file_operations trtl_cpu_dbg_fops = {
	.owner = THIS_MODULE,
	.open  = trtl_dbg_open,
	.release = trtl_dbg_close,
	.read = trtl_dbg_read,
	.poll = trtl_dbg_poll,
};

irqreturn_t trtl_irq_handler_debug(int irq_core_base, void *arg)
{
	struct fmc_device *fmc = arg;
	struct trtl_dev *trtl = fmc_get_drvdata(fmc);
	struct circ_buf *cb;
	uint32_t status;
	char c;
	int i;

	status = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_DBG_POLL);
do_irq:
	i = -1;
	while (status && ++i < trtl->n_cpu) {
		if (!(status & 0x1)) {
			status >>= 1;
			continue;
		}

		/* Select the CPU to use */
		spin_lock(&trtl->cpu[i].lock);
		fmc_writel(fmc, i, trtl->base_csr + WRN_CPU_CSR_REG_CORE_SEL);
		c = fmc_readl(fmc,
				trtl->base_csr + WRN_CPU_CSR_REG_DBG_MSG);
		cb = &trtl->cpu[i].cbuf;
		if (cb->buf) {
			/* We cans store the char */
			pr_debug("%s:%d %d=%c\n", __func__, __LINE__, i, c);
			cb->buf[cb->head] = c;
			cb->head = (cb->head + 1) & (dbg_max_msg - 1);
			if (cb->head == cb->tail) {
				cb->tail = (cb->tail + 1) & (dbg_max_msg - 1);
			}
		}
		spin_unlock(&trtl->cpu[i].lock);
	}

	status = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_DBG_POLL);
	if (status)
		goto do_irq;

	fmc_irq_ack(fmc);

	return IRQ_HANDLED;
}
