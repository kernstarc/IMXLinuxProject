#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>


#define CHARDEV_NAME "CV1813H_CHARDEV"
#define CHARDEV_MAJOR 200

static char read_buf[100];
static char write_buf[100];
static char kernel_data[] = {"Hello World!"};

static int chardev_open(struct inode *inode, struct file *filp)
{
	printk("chardev_open\r\n");
	return 0;
}

static ssize_t chardev_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	memcpy(read_buf, kernel_data, sizeof(kernel_data));
	ret = copy_to_user(buf, read_buf, cnt);
	if (ret == 0)
		printk("chrdev read success\r\n");
	else
		printk("chrdev read fail\r\n");
	return 0;	
}

static ssize_t chardev_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	ret = copy_from_user(write_buf, buf, cnt);
	if (ret == 0)
	{
		printk("chrdev write success\r\n");
		printk("kernel:%s\r\n", write_buf);
	}
	else
		printk("chrdev write fail\r\n");
	return 0;	
}

static int chardev_close(struct inode *inode, struct file *filp)
{
	printk("chardev_close\r\n");
	return 0;	
}

struct file_operations chardev_fops = {
	.owner = THIS_MODULE,
	.open = chardev_open,
	.read = chardev_read,
	.write = chardev_write,
	.release = chardev_close,
};


static int __init cv1813h_chardev_init(void)
{
	int ret = 0; 
	ret = register_chrdev(CHARDEV_MAJOR, CHARDEV_NAME, &chardev_fops);
	if (ret < 0)
	{
		printk("error register_chrdev\r\n");
		return -1;
	}
	printk("cv1813h_chardev_init OK 2\r\n");
	return 0;	
}

static void __exit cv1813h_chardev_exit(void)
{
	unregister_chrdev(CHARDEV_MAJOR, CHARDEV_NAME);
	printk("cv1813h_chardev_exit\r\n"); 
	return;
}


module_init(cv1813h_chardev_init)
module_exit(cv1813h_chardev_exit)

MODULE_AUTHOR("starc");
MODULE_LICENSE("GPL");





