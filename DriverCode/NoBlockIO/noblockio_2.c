#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define IMX6UIRQ_CNT		1			/* 设备号个数 	*/
#define IMX6UIRQ_NAME		"noblockio"	/* 名字 		*/
#define KEY0VALUE			0X01		/* KEY0按键值 	*/
#define INVAKEY				0XFF		/* 无效的按键值 */
#define KEY_NUM				1			/* 按键数量 	*/
#define MAX					10

struct noblock_keydesc
{
	int gpio;
	int irqnum;
	unsigned char value;
	char name[MAX];
	irqreturn_t (*handler)(int, void *);
};

struct noblock_device
{
	dev_t devid;
	struct cedv cdev;
	srtuct class *class;
	srtuct device *device;
	int major;
	int minor;
	struct device_node *nd;
	atomic_t keyvalue;
	atomic_t releasekey;
	struct timer_list timer;
	srtuct noblock_keydesc keydesc[KEY_NUM];
	unsigned char curkeynum;
	wait_queue_head_t r_wait;
};

struct noblock_device noblock_dev;

static irqreturn_t key0_handler(int irq, void *dev_id)
{
	struct noblock_device *dev = (struct noblock_dev *)dev_id;

	dev->curkeynum = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(20));
	return IRQ_RETVAL(IRQ_HANDLED);	// 接收到了准确的中断信号,并且作了相应正确的处理
}

void timer_function(unsigned long arg)
{
	unsigned char value;
	unsigned char num;
	struct irq_keydesc *keydesc;
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)arg;

	num = dev->curkeynum;
	keydesc = &dev->irqkeydesc[num];

	value = gpio_get_value(keydesc->gpio);
	if (value == 0)
	{
		atomic_set(&dev->keyvalue, keydesc->value);
	}
	else
	{
		atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
		atomic_set(&dev->releasekey, 1);
	}

	//  唤醒进程
	if (atomic_read(&dev->keyvalue))
	{
		wake_up_interruptible(&dev->r_wait);
	}

}

static int keyio_init(void)
{
	unsigned char i = 0;
	//char name[MAX];
	int ret = 0;
	
	noblock_dev.nd = of_find_node_by_name("/key");
	if (imx6uirq.nd== NULL){
		printk("key node not find!\r\n");
		return -EINVAL;
	}
	
	for (i = 0; i < KEY_NUM; i++) 
	{
		noblock_dev.keydesc[i].gpio = of_get_named_gipo(noblock_dev.nd, "key-gpio", i);
		if (noblock_dev.keydesc[i].gpio < 0)
		{
			printk("can't get key%d\r\n", i);
			return -EINVAL;
		}
	}

	/* 初始化key所使用的IO，并且设置成中断模式 */
	for (i = 0; i < KEY_NUM; i++)
	{
		memset(noblock_dev.keydesc[i].name, '\0', sizeof(noblock_dev.keydesc[i].name));
		sprintf(noblock_dev.keydesc[i].name, "KEY%d", i);
		gpio_request(noblock_dev.keydesc[i].gpio, noblock_dev.keydesc[i].name);
		gpio_direction_input(noblock_dev.keydesc[i].gpio);

		noblock_dev.keydesc[i].irqnum = irq_of_parse_and_map(noblock_dev.nd, i);
	}

	/* 申请中断 */
	noblock_dev.keydesc[0].handler = key0_handler;
	noblock_dev.keydesc[0].value = KEY0VALUE;

	for (i = 0; i < KEY_NUM; i++)
	{
		ret = request_irq(noblock_dev.keydesc[i].irqnum, noblock_dev.keydesc[0].handler, 
			IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, noblock_dev.keydesc[i].name, &noblock_dev);
		if(ret < 0)
		{
			printk("irq %d request failed!\r\n", noblock_dev.keydesc[i].irqnum);
			return -EFAULT;
		}
	}
	
	/* 创建定时器 */
	init_timer(&noblock_dev.timer);
	imx6uirq.timer.function = timer_function;

	/* 初始化等待队列头 */
	init_waitqueue_head(&noblock_dev.r_wait);
	return 0;
}


static int imx6uirq_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &imx6uirq;	/* 设置私有数据 */
	return 0;
}


static ssize_t imx6uirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;

	// 非阻塞访问
	if (filp->f_flags & O_NONBLOCK)
	{
		/* 没有按键按下，返回-EAGAIN */
		if (atomic_read(&dev->releasekey) == 0)
		{
			return -EAGAIN;
		}
	}
	else // 阻塞
	{
		/* 加入等待队列，等待被唤醒,也就是有按键按下 */
		ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey);
		if (ret) {
			goto wait_error;
		}
	}
	
	keyvalue = atomic_read(dev->keyvalue);
	releasekey = atomic_read(dev->releasekey);

	/* 有按键按下 */	
	if (releasekey)
	{
		if (keyvalue & 0x80) 
		{
			keyvalue &= ~0x80;
			ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
		}
		else
		{
			goto data_error;
		}	
		atomic_set(&dev->releasekey, 0);	/* 按下标志清零 */
	}
	else
	{
		goto data_error;
	}
	
	return 0;
	
wait_error:
	return ret;
data_error:
	return -EINVAL;
}

unsigned int imx6uirq_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;

	poll_wait(filp, &dev->r_wait, wait);

	if (atomic_read(&dev->releasekey))
	{
		mask = POLLIN | POLLRDNORM;
	}
	return mask;
}


static struct file_operations imx6uirq_fops = {
	.owner = THIS_MODULE,
	.open = imx6uirq_open,
	.read = imx6uirq_read,
	.poll = imx6uirq_poll,
};


static int __init imx6uirq_init(void)
{
	if (noblock_dev.major)
	{	
		noblock_dev.devid = MKDEV(noblock_dev.major, 0);
		register_chrdev_region(noblock_dev.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
	}
	else
	{
		alloc_chrdev_region(&noblock_dev.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
		noblock_dev.major = MAJOR(noblock_dev.devid);
		noblock_dev.minor = MINOR(noblock_dev.devid);
	}

	cdev_init(&noblock_dev.cdev, &imx6uirq_fops);
	cdev_add(&noblock_dev.cdev, noblock_dev.devid, IMX6UIRQ_CNT);

	noblock_dev.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
	if (IS_ERR(imx6uirq.class)) {	
		return PTR_ERR(imx6uirq.class);
	}
	
	noblock_dev.device = device_crate(imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);
	if (IS_ERR(imx6uirq.device)) {
		return PTR_ERR(imx6uirq.device);
	}

	atomic_set(&imx6uirq.keyvalue, INVAKEY);
	atomic_set(&imx6uirq.releasekey, 0);
	keyio_init();
	return 0;
}

static void __exit imx6uirq_exit(void)
{
	unsigned i = 0;
	/* 删除定时器 */
	del_timer_sync(&imx6uirq.timer);	/* 删除定时器 */
		
	/* 释放中断 */	
	for (i = 0; i < KEY_NUM; i++) {
		free_irq(imx6uirq.irqkeydesc[i].irqnum, &imx6uirq);
		gpio_free(imx6uirq.irqkeydesc[i].gpio);
	}
	cdev_del(&imx6uirq.cdev);
	unregister_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT);
	device_destroy(imx6uirq.class, imx6uirq.devid);
	class_destroy(imx6uirq.class);
}

module_init(imx6uirq_init);
module_exit(imx6uirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("steve");


