// SPDX-License-Identifier: GPL-2.0-only

#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Afif Rafiqin Adnan");
MODULE_DESCRIPTION("STACK MODULE using Misc Device + C");

#define DEVICE_NAME "stack_module_c"
#define BUFFER_SIZE 127
#define INSERT_VALUE _IOR('|', 1, int)
#define POP_VALUE _IOW('|', 2, int)

struct device_buffer {
	unsigned char buffer[BUFFER_SIZE];
	size_t pointer;
	struct mutex lock;
};

static struct device_buffer dev_buffer = { .buffer = { 0 },
					   .pointer = 0,
					   .lock = __MUTEX_INITIALIZER(
						   dev_buffer.lock) };

static long stack_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned char value;
	long retval = 0;

	mutex_lock(&dev_buffer.lock);

	switch (cmd) {
	case INSERT_VALUE:
		if (copy_from_user(&value, (int __user *)arg,
				   sizeof(unsigned char))) {
			pr_err("Failed to read value from user\n");
			retval = -EFAULT;
			break;
		}

		dev_buffer.buffer[dev_buffer.pointer++] = value;
		pr_info("Value inserted: %d\n", value);
		break;

	case POP_VALUE:
		value = dev_buffer.buffer[--dev_buffer.pointer];
		if (copy_to_user((int __user *)arg, &value,
				 sizeof(unsigned char))) {
			pr_err("Failed to write value to user\n");
			retval = -EFAULT;
			break;
		}

		pr_info("Value popped: %d\n", value);
		break;

	default:
		pr_err("IOCTL command not recognized: %u\n", cmd);
		retval = -ENOTTY;
	}

	mutex_unlock(&dev_buffer.lock);
	return retval;
}

static int stack_open(struct inode *inode, struct file *file)
{
	mutex_lock(&dev_buffer.lock);
	memset(dev_buffer.buffer, 0, BUFFER_SIZE);
	dev_buffer.pointer = 0;
	mutex_unlock(&dev_buffer.lock);
	pr_info("stack device opened\n");
	return 0;
}

static int stack_release(struct inode *inode, struct file *file)
{
	pr_info("stack device released\n");
	return 0;
}

static const struct file_operations stack_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = stack_ioctl,
	.open = stack_open,
	.release = stack_release,
};

static struct miscdevice stack_module_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &stack_fops,
};

static int __init stack_init(void)
{
	int ret;

	ret = misc_register(&stack_module_device);
	if (ret) {
		pr_err("Failed to register misc device\n");
		return ret;
	}

	pr_info("stack misc device registered\n");
	return 0;
}

static void __exit stack_exit(void)
{
	misc_deregister(&stack_module_device);
	pr_info("stack misc device unregistered\n");
}

module_init(stack_init);
module_exit(stack_exit);