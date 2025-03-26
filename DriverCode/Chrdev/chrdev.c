#include <linux/types.h>
#include <linux/init.h>  
#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>

#define CHRDEV_MAJOR	200
#define CHRDEV_NAME		"chrdev_module"

static char readbuf[100];
static char writebuf[100];
static char kerneldata[] = {"Hello World!"};


static int chrdev_open(struct inode *inode, struct file *filp)
{
	printk("chrdev module open");
	return 0;
}

static ssize_t chrdev_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue = 0;

	memcpy(readbuf, kerneldata, sizeof(kerneldata));
	retvalue = copy_to_user(buf, readbuf, cnt);
	if (retvalue == 0)
	{
		printk("chrdev read success\r\n");
	} else {
		printk("chrdev read fail\r\n");
	}
	printk("chrdevbase read!\r\n");
	return 0;
}

static ssize_t chrdev_write(struct file *flip, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retValue = 0;
	retValue = copy_from_user(writebuf, buf, cnt);
	if (retValue == 0)
	{
		printk("chrdev write success:%s\r\n", writebuf);
	} else {
		printk("chrdev write fail\r\n");
	}
	printk("chrdevbase write!\r\n");
	return 0;
}

static int chrdev_release(struct inode *inode, struct file *filp)
{
	printk("unload chrdev module");
	return 0;
}

static struct file_operations chrdev_fops = {
	.owner = THIS_MODULE,
	.open = chrdev_open,
	.read = chrdev_read,
	.write = chrdev_write,
	.release = chrdev_release,
};

static int __init chrdev_init(void)  
{  
	int retValue = 0;
	printk("load module\n");  

	retValue = register_chrdev(CHRDEV_MAJOR, CHRDEV_NAME, &chrdev_fops);
	if (retValue < 0)
	{
		printk("error register_chrdev");
	}
	printk("chrdevbase_init()\r\n"); 
    return 0;  
}  
 
static void __exit chrdev_exit(void)  
{  
    printk("unload module\n");  
	unregister_chrdev(CHRDEV_MAJOR, CHRDEV_NAME);
	printk("chrdevbase_exit()\r\n"); 
    return;  
}  
 
module_init(chrdev_init);  
module_exit(chrdev_exit);  
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("steve");

