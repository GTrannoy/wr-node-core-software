/*
 * Copyright (C) 2014 CERN (www.cern.ch)
 * Author: Federico Vaga <federico.vaga@cern.ch>
 * License: GPL v2
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/fmc.h>
#include <hw/wrn_cpu_csr.h>

#include "wrnc.h"


/**
 * Set the reset bit of the CPUs according to the mask
 */
void wrnc_cpu_reset_set(struct wrnc_dev *wrnc, uint8_t mask)
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

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	if (val)
		wrnc_cpu_reset_set(wrnc, (1 << cpu->index));
	else
		wrnc_cpu_reset_clr(wrnc, (1 << cpu->index));

	return count;
}



/**
 * Set the reset bit of the CPUs according to the mask
 * NOTE : for the CPU 1 means pause
 */
void wrnc_cpu_enable_set(struct wrnc_dev *wrnc, uint8_t mask)
{
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t reg_val;

	reg_val = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);
	reg_val |= (mask & 0xFF);
	fmc_writel(fmc, reg_val, wrnc->base_csr + WRN_CPU_CSR_REG_ENABLE);
}

/**
 * Clear the reset bit of the CPUs according to the mask
 * NOTE : for the CPU 0 means run
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

	if (kstrtol(buf, 0, &val))
		return -EINVAL;

	if (val)
		wrnc_cpu_enable_set(wrnc, (1 << cpu->index));
	else
		wrnc_cpu_enable_clr(wrnc, (1 << cpu->index));

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

const struct attribute_group *wrnc_cpu_groups[] = {
	&wrnc_cpu_group,
	NULL,
};


/**
 * It loads a given application into the CPU memory
 */
static int wrnc_cpu_firmware_load(struct wrnc_cpu *cpu, void *fw_buf,
				  size_t count, loff_t off)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(cpu->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t *fw = fw_buf, word, word_rb;
	int size, offset, i;

	if (off + count > WRNC_CPU_MEM_SIZE_BYTE) {
		dev_err(&cpu->dev,
			"Cannot load firmware: size limit %d byte\n",
			WRNC_CPU_MEM_SIZE_BYTE);
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
	/* FIXME get size dynamically*/
	for (i = offset; i < WRNC_CPU_MEM_SIZE_WORD; ++i) {
		fmc_writel(fmc, i, wrnc->base_csr + WRN_CPU_CSR_REG_UADDR);
		fmc_writel(fmc, 0, wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
	}

	/* Load the firmware */
	for (i = 0; i < size; ++i) {
		word = cpu_to_be32(fw[i]);
		fmc_writel(fmc, i + offset,
			   wrnc->base_csr + WRN_CPU_CSR_REG_UADDR);
		fmc_writel(fmc, word, wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
		word_rb = fmc_readl(fmc,
				    wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
		if (word != word_rb) {
			dev_err(&cpu->dev, "failed to load firmware\n");
			return -EFAULT;
		}
	}

	return 0;
}

static int wrnc_cpu_firmware_dump(struct wrnc_cpu *cpu, void *fw_buf,
				  size_t count, loff_t off)
{
	struct wrnc_dev *wrnc = to_wrnc_dev(cpu->dev.parent);
	struct fmc_device *fmc = to_fmc_dev(wrnc);
	uint32_t *fw = fw_buf, word;
	int size, offset, i;

	if (off + count > WRNC_CPU_MEM_SIZE_BYTE) {
		dev_err(&cpu->dev, "Cannot dump firmware: size limit %d byte\n",
			WRNC_CPU_MEM_SIZE_BYTE);
		return -ENOMEM;
	}

	/* Calculate code size in 32bit word*/
	size = (count + 3) / 4;
	offset = off / 4;

	/* Select the CPU memory to write */
	fmc_writel(fmc, cpu->index, wrnc->base_csr + WRN_CPU_CSR_REG_CORE_SEL);

	/* Dump the firmware */
	for (i = 0; i < size; ++i) {
		fmc_writel(fmc, i + offset,
			   wrnc->base_csr + WRN_CPU_CSR_REG_UADDR);
		word = fmc_readl(fmc, wrnc->base_csr + WRN_CPU_CSR_REG_UDATA);
		fw[i] = be32_to_cpu(word);
	}

	return 0;
}


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

out_cpy:
out_dmp:
	vfree(lbuf);
	return err ? err : count;
}

const struct file_operations wrnc_cpu_fops = {
	.owner = THIS_MODULE,
	.open  = wrnc_cpu_simple_open,
	.write  = wrnc_cpu_write,
	.read = wrnc_cpu_read,
	.llseek = generic_file_llseek,
};
