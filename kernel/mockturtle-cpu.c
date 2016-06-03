/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>

#include <linux/fmc.h>
#include <hw/mockturtle_cpu_csr.h>

#include "mockturtle-drv.h"


/**
 * Set the reset bit of the CPUs according to the mask
 */
void trtl_cpu_reset_set(struct trtl_dev *trtl, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_RESET);
	reg_val |= (mask & 0xFF);
	fmc_writel(fmc, reg_val, trtl->base_csr + WRN_CPU_CSR_REG_RESET);
}

/**
 * Clear the reset bit of the CPUs according to the mask
 */
static void trtl_cpu_reset_clr(struct trtl_dev *trtl, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_RESET);
	reg_val &= (~mask & 0xFF);
	fmc_writel(fmc, reg_val, trtl->base_csr + WRN_CPU_CSR_REG_RESET);
}

/**
 * It returns the running status of the CPU
 */
static ssize_t trtl_show_reset(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct trtl_cpu *cpu = to_trtl_cpu(dev);
	struct trtl_dev *trtl = to_trtl_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_RESET);

	return sprintf(buf, "%d\n", !!(reg_val & (1 << cpu->index)));
}

/**
 * It run or pause the CPU
 */
static ssize_t trtl_store_reset(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct trtl_cpu *cpu = to_trtl_cpu(dev);
	struct trtl_dev *trtl = to_trtl_dev(dev->parent);
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	if (val)
		trtl_cpu_reset_set(trtl, (1 << cpu->index));
	else
		trtl_cpu_reset_clr(trtl, (1 << cpu->index));

	return count;
}



/**
 * Set the reset bit of the CPUs according to the mask
 * NOTE : for the CPU 1 means pause
 */
void trtl_cpu_enable_set(struct trtl_dev *trtl, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_ENABLE);
	reg_val |= (mask & 0xFF);
	fmc_writel(fmc, reg_val, trtl->base_csr + WRN_CPU_CSR_REG_ENABLE);
}

/**
 * Clear the reset bit of the CPUs according to the mask
 * NOTE : for the CPU 0 means run
 */
static void trtl_cpu_enable_clr(struct trtl_dev *trtl, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_ENABLE);
	reg_val &= (~mask & 0xFF);
	fmc_writel(fmc, reg_val, trtl->base_csr + WRN_CPU_CSR_REG_ENABLE);
}

/**
 * It returns the current enable status of the CPU
 */
static ssize_t trtl_show_enable(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct trtl_cpu *cpu = to_trtl_cpu(dev);
	struct trtl_dev *trtl = to_trtl_dev(dev->parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_ENABLE);

	return sprintf(buf, "%d\n", !!(reg_val & (1 << cpu->index)));
}

/**
 * It enable or disable the CPU
 */
static ssize_t trtl_store_enable(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	struct trtl_cpu *cpu = to_trtl_cpu(dev);
	struct trtl_dev *trtl = to_trtl_dev(dev->parent);
	long val;

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	if (val)
		trtl_cpu_enable_set(trtl, (1 << cpu->index));
	else
		trtl_cpu_enable_clr(trtl, (1 << cpu->index));

	return count;
}


DEVICE_ATTR(enable, (S_IRUGO | S_IWUSR), trtl_show_enable, trtl_store_enable);
DEVICE_ATTR(reset, (S_IRUGO | S_IWUSR), trtl_show_reset, trtl_store_reset);

static struct attribute *trtl_cpu_attr[] = {
	&dev_attr_enable.attr,
	&dev_attr_reset.attr,
	NULL,
};

static const struct attribute_group trtl_cpu_group = {
	.attrs = trtl_cpu_attr,
};

const struct attribute_group *trtl_cpu_groups[] = {
	&trtl_cpu_group,
	NULL,
};


/**
 * It loads a given application into the CPU memory
 */
static int trtl_cpu_firmware_load(struct trtl_cpu *cpu, void *fw_buf,
				  size_t count, loff_t off)
{
	struct trtl_dev *trtl = to_trtl_dev(cpu->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t *fw = fw_buf, word, word_rb;
	int size, offset, i, cpu_memsize;

	/* Select the CPU memory to write */
	fmc_writel(fmc, cpu->index, trtl->base_csr + WRN_CPU_CSR_REG_CORE_SEL);
	cpu_memsize = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_CORE_MEMSIZE );

	if (off + count > cpu_memsize) {
		dev_err(&cpu->dev,
			"Cannot load firmware: size limit %d byte\n",
			cpu_memsize);
		return -ENOMEM;
	}

	/* Calculate code size in 32bit word*/
	size = (count + 3) / 4;
	offset = off / 4;

	/* Reset the CPU before overwrite its memory */
	trtl_cpu_reset_set(trtl, (1 << cpu->index));

	/* Clean CPU memory */
	for (i = offset; i < cpu_memsize / 1024; ++i) {
		fmc_writel(fmc, i, trtl->base_csr + WRN_CPU_CSR_REG_UADDR);
		udelay(1);
		fmc_writel(fmc, 0, trtl->base_csr + WRN_CPU_CSR_REG_UDATA);
		udelay(1);
	}

	/* Load the firmware */
	for (i = 0; i < size; ++i) {
		word = cpu_to_be32(fw[i]);
		fmc_writel(fmc, i + offset,
			   trtl->base_csr + WRN_CPU_CSR_REG_UADDR);
		udelay(1);
		fmc_writel(fmc, word, trtl->base_csr + WRN_CPU_CSR_REG_UDATA);
		udelay(1);
		word_rb = fmc_readl(fmc,
				    trtl->base_csr + WRN_CPU_CSR_REG_UDATA);
		udelay(1);
		if (word != word_rb) {
			dev_err(&cpu->dev,
				"failed to load firmware (byte %d | 0x%x != 0x%x)\n",
				i, word, word_rb);
			return -EFAULT;
		}
	}

	return 0;
}

static int trtl_cpu_firmware_dump(struct trtl_cpu *cpu, void *fw_buf,
				  size_t count, loff_t off)
{
	struct trtl_dev *trtl = to_trtl_dev(cpu->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(trtl);
	uint32_t *fw = fw_buf, word;
	int size, offset, i, cpu_memsize;


	/* Calculate code size in 32bit word*/
	size = (count + 3) / 4;
	offset = off / 4;

	/* Select the CPU memory to write */
	fmc_writel(fmc, cpu->index, trtl->base_csr + WRN_CPU_CSR_REG_CORE_SEL);

	cpu_memsize = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_CORE_MEMSIZE );

	if (off + count > cpu_memsize) {
		dev_err(&cpu->dev, "Cannot dump firmware: size limit %d byte\n",
			cpu_memsize);
		return -ENOMEM;
	}

	/* Dump the firmware */
	for (i = 0; i < size; ++i) {
		fmc_writel(fmc, i + offset,
			   trtl->base_csr + WRN_CPU_CSR_REG_UADDR);
		udelay(1);
		word = fmc_readl(fmc, trtl->base_csr + WRN_CPU_CSR_REG_UDATA);
		udelay(1);
		fw[i] = be32_to_cpu(word);
	}

	return 0;
}


static int trtl_cpu_simple_open(struct inode *inode, struct file *file)
{
	int m = iminor(inode);

	file->private_data = to_trtl_cpu(minors[m]);

	return 0;
}

/**
 * It writes a given firmware into a CPU
 */
static ssize_t trtl_cpu_write(struct file *f, const char __user *buf,
			      size_t count, loff_t *offp)
{
	struct trtl_cpu *cpu = f->private_data;
	void *lbuf;
	int err;

	if (!count)
		return -EINVAL;

	lbuf = vmalloc(count);
	if (!lbuf)
		return -ENOMEM;
	if (copy_from_user(lbuf, buf, count)) {
		err = -EFAULT;
		goto out_cpy;
	}

	err = trtl_cpu_firmware_load(cpu, lbuf, count, *offp);
	if (err)
		goto out_load;

	*offp += count;

out_load:
out_cpy:
	vfree(lbuf);
	return err ? err : count;
}

/**
 * It reads the firmware from a CPU
 */
static ssize_t trtl_cpu_read(struct file *f, char __user *buf,
			     size_t count, loff_t *offp)
{
	struct trtl_cpu *cpu = f->private_data;
	void *lbuf;
	int err;

	lbuf = vmalloc(count);
	if (!lbuf)
		return -ENOMEM;

	err = trtl_cpu_firmware_dump(cpu, lbuf, count, *offp);
	if (err)
		goto out_dmp;

	if (copy_to_user(buf, lbuf, count)) {
		err = -EFAULT;
		goto out_cpy;
	}

	*offp += count;

out_cpy:
out_dmp:
	vfree(lbuf);
	return err ? err : count;
}

const struct file_operations trtl_cpu_fops = {
	.owner = THIS_MODULE,
	.open  = trtl_cpu_simple_open,
	.write  = trtl_cpu_write,
	.read = trtl_cpu_read,
	.llseek = generic_file_llseek,
};
