/*
 * LIRC base driver
 *
 * by Artur Lipowski <alipowski@interia.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/sched/signal.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include <media/rc-core.h>
#include <media/lirc.h>
#include <media/lirc_dev.h>

#define NOPLUG		-1
#define LOGHEAD		"lirc_dev (%s[%d]): "
#define LIRCBUF_SIZE    1024

static dev_t lirc_base_dev;

struct irctl {
	struct lirc_driver d;
	int attached;
	int open;

	struct mutex irctl_lock;
	struct lirc_buffer *buf;
	bool buf_internal;
	unsigned int chunk_size;

	struct device dev;
	struct cdev cdev;
};

static DEFINE_MUTEX(lirc_dev_lock);

static struct irctl *irctls[MAX_IRCTL_DEVICES];

/* Only used for sysfs but defined to void otherwise */
static struct class *lirc_class;


static void lirc_release(struct device *ld)
{
	struct irctl *ir = container_of(ld, struct irctl, dev);

	put_device(ir->dev.parent);

	if (ir->buf_internal) {
		lirc_buffer_free(ir->buf);
		kfree(ir->buf);
	}

	mutex_lock(&lirc_dev_lock);
	irctls[ir->d.minor] = NULL;
	mutex_unlock(&lirc_dev_lock);
	kfree(ir);
}

static int lirc_allocate_buffer(struct irctl *ir)
{
	int err = 0;
	int bytes_in_key;
	unsigned int chunk_size;
	unsigned int buffer_size;
	struct lirc_driver *d = &ir->d;

	bytes_in_key = BITS_TO_LONGS(d->code_length) +
						(d->code_length % 8 ? 1 : 0);
	buffer_size = d->buffer_size ? d->buffer_size : BUFLEN / bytes_in_key;
	chunk_size  = d->chunk_size  ? d->chunk_size  : bytes_in_key;

	if (d->rbuf) {
		ir->buf = d->rbuf;
		ir->buf_internal = false;
	} else {
		ir->buf = kmalloc(sizeof(struct lirc_buffer), GFP_KERNEL);
		if (!ir->buf) {
			err = -ENOMEM;
			goto out;
		}

		err = lirc_buffer_init(ir->buf, chunk_size, buffer_size);
		if (err) {
			kfree(ir->buf);
			ir->buf = NULL;
			goto out;
		}

		ir->buf_internal = true;
		d->rbuf = ir->buf;
	}
	ir->chunk_size = ir->buf->chunk_size;

out:
<<<<<<< HEAD
=======
	mutex_unlock(&dev->lock);
	return ret;
}

static __poll_t ir_lirc_poll(struct file *file, struct poll_table_struct *wait)
{
	struct lirc_fh *fh = file->private_data;
	struct rc_dev *rcdev = fh->rc;
	__poll_t events = 0;

	poll_wait(file, &fh->wait_poll, wait);

	if (!rcdev->registered) {
		events = EPOLLHUP | EPOLLERR;
	} else if (rcdev->driver_type != RC_DRIVER_IR_RAW_TX) {
		if (fh->rec_mode == LIRC_MODE_SCANCODE &&
		    !kfifo_is_empty(&fh->scancodes))
			events = EPOLLIN | EPOLLRDNORM;

		if (fh->rec_mode == LIRC_MODE_MODE2 &&
		    !kfifo_is_empty(&fh->rawir))
			events = EPOLLIN | EPOLLRDNORM;
	}

	return events;
}

static ssize_t ir_lirc_read_mode2(struct file *file, char __user *buffer,
				  size_t length)
{
	struct lirc_fh *fh = file->private_data;
	struct rc_dev *rcdev = fh->rc;
	unsigned int copied;
	int ret;

	if (length < sizeof(unsigned int) || length % sizeof(unsigned int))
		return -EINVAL;

	do {
		if (kfifo_is_empty(&fh->rawir)) {
			if (file->f_flags & O_NONBLOCK)
				return -EAGAIN;

			ret = wait_event_interruptible(fh->wait_poll,
					!kfifo_is_empty(&fh->rawir) ||
					!rcdev->registered);
			if (ret)
				return ret;
		}

		if (!rcdev->registered)
			return -ENODEV;

		ret = mutex_lock_interruptible(&rcdev->lock);
		if (ret)
			return ret;
		ret = kfifo_to_user(&fh->rawir, buffer, length, &copied);
		mutex_unlock(&rcdev->lock);
		if (ret)
			return ret;
	} while (copied == 0);

	return copied;
}

static ssize_t ir_lirc_read_scancode(struct file *file, char __user *buffer,
				     size_t length)
{
	struct lirc_fh *fh = file->private_data;
	struct rc_dev *rcdev = fh->rc;
	unsigned int copied;
	int ret;

	if (length < sizeof(struct lirc_scancode) ||
	    length % sizeof(struct lirc_scancode))
		return -EINVAL;

	do {
		if (kfifo_is_empty(&fh->scancodes)) {
			if (file->f_flags & O_NONBLOCK)
				return -EAGAIN;

			ret = wait_event_interruptible(fh->wait_poll,
					!kfifo_is_empty(&fh->scancodes) ||
					!rcdev->registered);
			if (ret)
				return ret;
		}

		if (!rcdev->registered)
			return -ENODEV;

		ret = mutex_lock_interruptible(&rcdev->lock);
		if (ret)
			return ret;
		ret = kfifo_to_user(&fh->scancodes, buffer, length, &copied);
		mutex_unlock(&rcdev->lock);
		if (ret)
			return ret;
	} while (copied == 0);

	return copied;
}

static ssize_t ir_lirc_read(struct file *file, char __user *buffer,
			    size_t length, loff_t *ppos)
{
	struct lirc_fh *fh = file->private_data;
	struct rc_dev *rcdev = fh->rc;

	if (rcdev->driver_type == RC_DRIVER_IR_RAW_TX)
		return -EINVAL;

	if (!rcdev->registered)
		return -ENODEV;

	if (fh->rec_mode == LIRC_MODE_MODE2)
		return ir_lirc_read_mode2(file, buffer, length);
	else /* LIRC_MODE_SCANCODE */
		return ir_lirc_read_scancode(file, buffer, length);
}

static const struct file_operations lirc_fops = {
	.owner		= THIS_MODULE,
	.write		= ir_lirc_transmit_ir,
	.unlocked_ioctl	= ir_lirc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ir_lirc_ioctl,
#endif
	.read		= ir_lirc_read,
	.poll		= ir_lirc_poll,
	.open		= ir_lirc_open,
	.release	= ir_lirc_close,
	.llseek		= no_llseek,
};

static void lirc_release_device(struct device *ld)
{
	struct rc_dev *rcdev = container_of(ld, struct rc_dev, lirc_dev);

	put_device(&rcdev->dev);
}

int ir_lirc_register(struct rc_dev *dev)
{
	const char *rx_type, *tx_type;
	int err, minor;

	minor = ida_alloc_max(&lirc_ida, RC_DEV_MAX - 1, GFP_KERNEL);
	if (minor < 0)
		return minor;

	device_initialize(&dev->lirc_dev);
	dev->lirc_dev.class = lirc_class;
	dev->lirc_dev.parent = &dev->dev;
	dev->lirc_dev.release = lirc_release_device;
	dev->lirc_dev.devt = MKDEV(MAJOR(lirc_base_dev), minor);
	dev_set_name(&dev->lirc_dev, "lirc%d", minor);

	INIT_LIST_HEAD(&dev->lirc_fh);
	spin_lock_init(&dev->lirc_fh_lock);

	cdev_init(&dev->lirc_cdev, &lirc_fops);

	get_device(&dev->dev);

	err = cdev_device_add(&dev->lirc_cdev, &dev->lirc_dev);
	if (err)
		goto out_put_device;

	switch (dev->driver_type) {
	case RC_DRIVER_SCANCODE:
		rx_type = "scancode";
		break;
	case RC_DRIVER_IR_RAW:
		rx_type = "raw IR";
		break;
	default:
		rx_type = "no";
		break;
	}

	if (dev->tx_ir)
		tx_type = "raw IR";
	else
		tx_type = "no";

	dev_info(&dev->dev, "lirc_dev: driver %s registered at minor = %d, %s receiver, %s transmitter",
		 dev->driver_name, minor, rx_type, tx_type);

	return 0;

out_put_device:
	put_device(&dev->lirc_dev);
	ida_free(&lirc_ida, minor);
>>>>>>> upstream/linux-4.19.y-cip
	return err;
}

int lirc_register_driver(struct lirc_driver *d)
{
	struct irctl *ir;
	int minor;
	int err;

	if (!d) {
		pr_err("driver pointer must be not NULL!\n");
		return -EBADRQC;
	}

	if (!d->dev) {
		pr_err("dev pointer not filled in!\n");
		return -EINVAL;
	}

<<<<<<< HEAD
	if (!d->fops) {
		pr_err("fops pointer not filled in!\n");
		return -EINVAL;
	}

	if (d->minor >= MAX_IRCTL_DEVICES) {
		dev_err(d->dev, "minor must be between 0 and %d!\n",
						MAX_IRCTL_DEVICES - 1);
		return -EBADRQC;
	}

	if (d->code_length < 1 || d->code_length > (BUFLEN * 8)) {
		dev_err(d->dev, "code length must be less than %d bits\n",
								BUFLEN * 8);
		return -EBADRQC;
	}

	if (!d->rbuf && !(d->fops && d->fops->read &&
			  d->fops->poll && d->fops->unlocked_ioctl)) {
		dev_err(d->dev, "undefined read, poll, ioctl\n");
		return -EBADRQC;
	}

	mutex_lock(&lirc_dev_lock);

	minor = d->minor;

	if (minor < 0) {
		/* find first free slot for driver */
		for (minor = 0; minor < MAX_IRCTL_DEVICES; minor++)
			if (!irctls[minor])
				break;
		if (minor == MAX_IRCTL_DEVICES) {
			dev_err(d->dev, "no free slots for drivers!\n");
			err = -ENOMEM;
			goto out_lock;
		}
	} else if (irctls[minor]) {
		dev_err(d->dev, "minor (%d) just registered!\n", minor);
		err = -EBUSY;
		goto out_lock;
	}

	ir = kzalloc(sizeof(struct irctl), GFP_KERNEL);
	if (!ir) {
		err = -ENOMEM;
		goto out_lock;
	}

	mutex_init(&ir->irctl_lock);
	irctls[minor] = ir;
	d->minor = minor;

	/* some safety check 8-) */
	d->name[sizeof(d->name)-1] = '\0';

	if (d->features == 0)
		d->features = LIRC_CAN_REC_LIRCCODE;

	ir->d = *d;

	if (LIRC_CAN_REC(d->features)) {
		err = lirc_allocate_buffer(irctls[minor]);
		if (err) {
			kfree(ir);
			goto out_lock;
		}
		d->rbuf = ir->buf;
	}

	device_initialize(&ir->dev);
	ir->dev.devt = MKDEV(MAJOR(lirc_base_dev), ir->d.minor);
	ir->dev.class = lirc_class;
	ir->dev.parent = d->dev;
	ir->dev.release = lirc_release;
	dev_set_name(&ir->dev, "lirc%d", ir->d.minor);

	cdev_init(&ir->cdev, d->fops);
	ir->cdev.owner = ir->d.owner;
	ir->cdev.kobj.parent = &ir->dev.kobj;

	err = cdev_add(&ir->cdev, ir->dev.devt, 1);
	if (err)
		goto out_free_dev;

	ir->attached = 1;

	err = device_add(&ir->dev);
	if (err)
		goto out_cdev;

	mutex_unlock(&lirc_dev_lock);

	get_device(ir->dev.parent);

	dev_info(ir->d.dev, "lirc_dev: driver %s registered at minor = %d\n",
		 ir->d.name, ir->d.minor);

	return minor;

out_cdev:
	cdev_del(&ir->cdev);
out_free_dev:
	put_device(&ir->dev);
out_lock:
	mutex_unlock(&lirc_dev_lock);

	return err;
=======
	cdev_device_del(&dev->lirc_cdev, &dev->lirc_dev);
	ida_free(&lirc_ida, MINOR(dev->lirc_dev.devt));
>>>>>>> upstream/linux-4.19.y-cip
}
EXPORT_SYMBOL(lirc_register_driver);

int lirc_unregister_driver(int minor)
{
	struct irctl *ir;

	if (minor < 0 || minor >= MAX_IRCTL_DEVICES) {
		pr_err("minor (%d) must be between 0 and %d!\n",
					minor, MAX_IRCTL_DEVICES - 1);
		return -EBADRQC;
	}

	ir = irctls[minor];
	if (!ir) {
		pr_err("failed to get irctl\n");
		return -ENOENT;
	}

	mutex_lock(&lirc_dev_lock);

	if (ir->d.minor != minor) {
		dev_err(ir->d.dev, "lirc_dev: minor %d device not registered\n",
									minor);
		mutex_unlock(&lirc_dev_lock);
		return -ENOENT;
	}

	dev_dbg(ir->d.dev, "lirc_dev: driver %s unregistered from minor = %d\n",
		ir->d.name, ir->d.minor);

	ir->attached = 0;
	if (ir->open) {
		dev_dbg(ir->d.dev, LOGHEAD "releasing opened driver\n",
			ir->d.name, ir->d.minor);
		wake_up_interruptible(&ir->buf->wait_poll);
	}

	mutex_unlock(&lirc_dev_lock);

	device_del(&ir->dev);
	cdev_del(&ir->cdev);
	put_device(&ir->dev);

	return 0;
}
EXPORT_SYMBOL(lirc_unregister_driver);

int lirc_dev_fop_open(struct inode *inode, struct file *file)
{
	struct irctl *ir;
	int retval = 0;

	if (iminor(inode) >= MAX_IRCTL_DEVICES) {
		pr_err("open result for %d is -ENODEV\n", iminor(inode));
		return -ENODEV;
	}

	if (mutex_lock_interruptible(&lirc_dev_lock))
		return -ERESTARTSYS;

	ir = irctls[iminor(inode)];
	mutex_unlock(&lirc_dev_lock);

	if (!ir) {
		retval = -ENODEV;
		goto error;
	}

	dev_dbg(ir->d.dev, LOGHEAD "open called\n", ir->d.name, ir->d.minor);

	if (ir->d.minor == NOPLUG) {
		retval = -ENODEV;
		goto error;
	}

	if (ir->open) {
		retval = -EBUSY;
		goto error;
	}

	if (ir->d.rdev) {
		retval = rc_open(ir->d.rdev);
		if (retval)
			goto error;
	}

	if (ir->buf)
		lirc_buffer_clear(ir->buf);

	ir->open++;

error:
	nonseekable_open(inode, file);

	return retval;
}
EXPORT_SYMBOL(lirc_dev_fop_open);

int lirc_dev_fop_close(struct inode *inode, struct file *file)
{
	struct irctl *ir = irctls[iminor(inode)];
	int ret;

	if (!ir) {
		pr_err("called with invalid irctl\n");
		return -EINVAL;
	}

	ret = mutex_lock_killable(&lirc_dev_lock);
	WARN_ON(ret);

	rc_close(ir->d.rdev);

	ir->open--;
	if (!ret)
		mutex_unlock(&lirc_dev_lock);

	return 0;
}
EXPORT_SYMBOL(lirc_dev_fop_close);

unsigned int lirc_dev_fop_poll(struct file *file, poll_table *wait)
{
	struct irctl *ir = irctls[iminor(file_inode(file))];
	unsigned int ret;

	if (!ir) {
		pr_err("called with invalid irctl\n");
		return POLLERR;
	}

	if (!ir->attached)
		return POLLHUP | POLLERR;

	if (ir->buf) {
		poll_wait(file, &ir->buf->wait_poll, wait);

		if (lirc_buffer_empty(ir->buf))
			ret = 0;
		else
			ret = POLLIN | POLLRDNORM;
	} else
		ret = POLLERR;

	dev_dbg(ir->d.dev, LOGHEAD "poll result = %d\n",
		ir->d.name, ir->d.minor, ret);

	return ret;
}
EXPORT_SYMBOL(lirc_dev_fop_poll);

long lirc_dev_fop_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	__u32 mode;
	int result = 0;
	struct irctl *ir = irctls[iminor(file_inode(file))];

	if (!ir) {
		pr_err("no irctl found!\n");
		return -ENODEV;
	}

	dev_dbg(ir->d.dev, LOGHEAD "ioctl called (0x%x)\n",
		ir->d.name, ir->d.minor, cmd);

	if (ir->d.minor == NOPLUG || !ir->attached) {
		dev_err(ir->d.dev, LOGHEAD "ioctl result = -ENODEV\n",
			ir->d.name, ir->d.minor);
		return -ENODEV;
	}

	mutex_lock(&ir->irctl_lock);

	switch (cmd) {
	case LIRC_GET_FEATURES:
		result = put_user(ir->d.features, (__u32 __user *)arg);
		break;
	case LIRC_GET_REC_MODE:
		if (!LIRC_CAN_REC(ir->d.features)) {
			result = -ENOTTY;
			break;
		}

		result = put_user(LIRC_REC2MODE
				  (ir->d.features & LIRC_CAN_REC_MASK),
				  (__u32 __user *)arg);
		break;
	case LIRC_SET_REC_MODE:
		if (!LIRC_CAN_REC(ir->d.features)) {
			result = -ENOTTY;
			break;
		}

		result = get_user(mode, (__u32 __user *)arg);
		if (!result && !(LIRC_MODE2REC(mode) & ir->d.features))
			result = -EINVAL;
		/*
		 * FIXME: We should actually set the mode somehow but
		 * for now, lirc_serial doesn't support mode changing either
		 */
		break;
	case LIRC_GET_LENGTH:
		result = put_user(ir->d.code_length, (__u32 __user *)arg);
		break;
	case LIRC_GET_MIN_TIMEOUT:
		if (!(ir->d.features & LIRC_CAN_SET_REC_TIMEOUT) ||
		    ir->d.min_timeout == 0) {
			result = -ENOTTY;
			break;
		}

		result = put_user(ir->d.min_timeout, (__u32 __user *)arg);
		break;
	case LIRC_GET_MAX_TIMEOUT:
		if (!(ir->d.features & LIRC_CAN_SET_REC_TIMEOUT) ||
		    ir->d.max_timeout == 0) {
			result = -ENOTTY;
			break;
		}

		result = put_user(ir->d.max_timeout, (__u32 __user *)arg);
		break;
	default:
		result = -ENOTTY;
	}

	mutex_unlock(&ir->irctl_lock);

	return result;
}
EXPORT_SYMBOL(lirc_dev_fop_ioctl);

ssize_t lirc_dev_fop_read(struct file *file,
			  char __user *buffer,
			  size_t length,
			  loff_t *ppos)
{
	struct irctl *ir = irctls[iminor(file_inode(file))];
	unsigned char *buf;
	int ret = 0, written = 0;
	DECLARE_WAITQUEUE(wait, current);

	if (!ir) {
		pr_err("called with invalid irctl\n");
		return -ENODEV;
	}

	if (!LIRC_CAN_REC(ir->d.features))
		return -EINVAL;

	dev_dbg(ir->d.dev, LOGHEAD "read called\n", ir->d.name, ir->d.minor);

	buf = kzalloc(ir->chunk_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (mutex_lock_interruptible(&ir->irctl_lock)) {
		ret = -ERESTARTSYS;
		goto out_unlocked;
	}
	if (!ir->attached) {
		ret = -ENODEV;
		goto out_locked;
	}

	if (length % ir->chunk_size) {
		ret = -EINVAL;
		goto out_locked;
	}

	/*
	 * we add ourselves to the task queue before buffer check
	 * to avoid losing scan code (in case when queue is awaken somewhere
	 * between while condition checking and scheduling)
	 */
	add_wait_queue(&ir->buf->wait_poll, &wait);

	/*
	 * while we didn't provide 'length' bytes, device is opened in blocking
	 * mode and 'copy_to_user' is happy, wait for data.
	 */
	while (written < length && ret == 0) {
		if (lirc_buffer_empty(ir->buf)) {
			/* According to the read(2) man page, 'written' can be
			 * returned as less than 'length', instead of blocking
			 * again, returning -EWOULDBLOCK, or returning
			 * -ERESTARTSYS
			 */
			if (written)
				break;
			if (file->f_flags & O_NONBLOCK) {
				ret = -EWOULDBLOCK;
				break;
			}
			if (signal_pending(current)) {
				ret = -ERESTARTSYS;
				break;
			}

			mutex_unlock(&ir->irctl_lock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			set_current_state(TASK_RUNNING);

			if (mutex_lock_interruptible(&ir->irctl_lock)) {
				ret = -ERESTARTSYS;
				remove_wait_queue(&ir->buf->wait_poll, &wait);
				goto out_unlocked;
			}

			if (!ir->attached) {
				ret = -ENODEV;
				goto out_locked;
			}
		} else {
			lirc_buffer_read(ir->buf, buf);
			ret = copy_to_user((void __user *)buffer+written, buf,
					   ir->buf->chunk_size);
			if (!ret)
				written += ir->buf->chunk_size;
			else
				ret = -EFAULT;
		}
	}

	remove_wait_queue(&ir->buf->wait_poll, &wait);

out_locked:
	mutex_unlock(&ir->irctl_lock);

out_unlocked:
	kfree(buf);

	return ret ? ret : written;
}
EXPORT_SYMBOL(lirc_dev_fop_read);

void *lirc_get_pdata(struct file *file)
{
	return irctls[iminor(file_inode(file))]->d.data;
}
EXPORT_SYMBOL(lirc_get_pdata);


static int __init lirc_dev_init(void)
{
	int retval;

	lirc_class = class_create(THIS_MODULE, "lirc");
	if (IS_ERR(lirc_class)) {
		pr_err("class_create failed\n");
		return PTR_ERR(lirc_class);
	}

	retval = alloc_chrdev_region(&lirc_base_dev, 0, MAX_IRCTL_DEVICES,
				     "BaseRemoteCtl");
	if (retval) {
		class_destroy(lirc_class);
		pr_err("alloc_chrdev_region failed\n");
		return retval;
	}

	pr_info("IR Remote Control driver registered, major %d\n",
						MAJOR(lirc_base_dev));

	return 0;
}

static void __exit lirc_dev_exit(void)
{
	class_destroy(lirc_class);
	unregister_chrdev_region(lirc_base_dev, MAX_IRCTL_DEVICES);
	pr_info("module unloaded\n");
}

module_init(lirc_dev_init);
module_exit(lirc_dev_exit);

MODULE_DESCRIPTION("LIRC base driver module");
MODULE_AUTHOR("Artur Lipowski");
MODULE_LICENSE("GPL");
