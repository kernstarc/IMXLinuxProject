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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>                                                                          


#define IMX6UIRQ_CNT		1			/* 设备号个数 	*/
#define IMX6UIRQ_NAME		"blockio"	/* 名字 		*/
#define KEY0VALUE			0X01		/* KEY0按键值 	*/
#define INVAKEY				0XFF		/* 无效的按键值 */
#define KEY_NUM				1			/* 按键数量 	*/


// 中断IO结构体
struct irq_key {
	int gpio;
	int irqnum;
	unsigned char value;
	char name[20];
	irqreturn_t (*handler)(int, void *);
};

// 设备结构体
struct imx6ull_irq_dev {
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	int major;
	int minor;
	struct device_node *nd;
	atomic_t key_value;
	atomic_t release_value;
	struct timer_list timer;
	struct irq_key irq_key_desc[KEY_NUM];
	unsigned char curkeynum;
	wait_queue_head_t r_wait;
};

struct imx6ull_irq_dev imx6u_irq;

// 中断函数
static irqreturn_t key_handler(int irq, void *dev_id)
{
	struct imx6ull_irq_dev *dev = (struct imx6ull_irq_dev *)dev_id;

	dev->curkeynum = 0;
	dev->timer.data = (volatile long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));	/* 10ms定时 */

	return IRQ_RETVAL(IRQ_HANDLED);
}

// 定时器函数
static void timer_function_handler(unsigned long arg)
{
	unsigned char value;
	unsigned char num;
	struct irq_key *keydesc;
	struct imx6ull_irq_dev *dev = (struct imx6ull_irq_dev *)arg;

	num = dev->curkeynum;
	keydesc = dev->irq_key_desc[num];

	value = gpio_get_value(keydesc.gpio);
	if (value == 0)
	{
		atomic_set(&dev->key_value, keydesc->value);
	}
	else
	{
		atomic_set(&dev->key_value, 0x80 | keydesc->value);
		atomic_set(&dev->release_value, 1);	/* 标记松开按键，即完成一次完整的按键过程 */
	}

	/* 唤醒进程 */
	if(atomic_read(&dev->release_value)) {	/* 完成一次按键过程 */
		/* wake_up(&dev->r_wait); */
		wake_up_interruptible(&dev->r_wait);
	}
	
}

// 按键初始化函数
static int keyio_init(void)
{
	unsigned char i = 0;
	//char name[10];
	int ret = 0;

	imx6u_irq.nd = of_find_node_by_path("/key");
	if (imx6u_irq.nd== NULL){
		printk("key node not find!\r\n");
		return -EINVAL;
	} 

	/* 提取GPIO */
	for (i = 0; i < KEY_NUM; i++)
	{
		imx6u_irq.irq_key_desc[i].gpio = of_get_named_gpio(imx6u_irq.nd, "key-gpio", i);
		if (imx6u_irq.irq_key_desc[i].gpio < 0) {
			printk("can't get key%d\r\n", i);
		}
	}
	
	/* 初始化key所使用的IO，并且设置成中断模式 */
	for (i = 0; i < KEY_NUM; i++)
	{
		// 名字
		memset(imx6u_irq.irq_key_desc[i].name, '\0', sizeof(imx6u_irq.irq_key_desc[i].name));
		sprintf(imx6u_irq.irq_key_desc[i].name, "KEY%d", i);
		// gpio
		gpio_request(imx6u_irq.irq_key_desc[i].gpio, imx6u_irq.irq_key_desc[i].name);
		gpio_direction_input(imx6u_irq.irq_key_desc[i].gpio);
		// 中断号
		imx6u_irq.irq_key_desc[i].irqnum = irq_of_parse_and_map(imx6u_irq.nd, i);
#if 0
		imx6uirq.irqkeydesc[i].irqnum = gpio_to_irq(imx6uirq.irqkeydesc[i].gpio);
#endif
	}

	/* 申请中断 */
	imx6u_irq.irq_key_desc[0].handler = key_handler;
	imx6u_irq.irq_key_desc[0].value = KEY0VALUE;

	for (i = 0; i < KEY_NUM; i++)
	{
		ret = request_irq(imx6u_irq.irq_key_desc[i].irqnum, imx6u_irq.irq_key_desc[i].handler, 
			IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, imx6u_irq.irq_key_desc[i].name, &imx6u_irq)
		if(ret < 0){
			printk("irq %d request failed!\r\n", imx6u_irq.irq_key_desc[i].irqnum);
			return -EFAULT;
		}
	}

	// 设置定时器
	init_timer(&imx6u_irq.timer);
	imx6u_irq.timer.function = timer_function_handler;

	// 初始化等待队列头
	init_waitqueue_head(&imx6u_irq.r_wait);
	return 0;
}

static int imx6uirq_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &imx6u_irq;
	return 0;
}

static ssize_t imx6uirq_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0;
	unsigned char releasekey = 0;
	struct imx6uirq_dev *dev = (struct imx6uirq_dev *)filp->private_data;

#if 0
	/* 加入等待队列，等待被唤醒,也就是有按键按下 */
 	ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey)); 
	if (ret) {
		goto wait_error;
	} 
#endif

	DECLARE_WAITQUEUE(wait, current);	/* 定义一个等待队列 */
	if(atomic_read(&dev->releasekey) == 0) 
	{	/* 没有按键按下 */
		add_wait_queue(&dev->r_wait, &wait);	/* 将等待队列添加到等待队列头 */
		__set_current_state(TASK_INTERRUPTIBLE);/* 设置任务状态 */
		schedule();							/* 进行一次任务切换 */
		if(signal_pending(current))	{			/* 判断是否为信号引起的唤醒 */
			ret = -ERESTARTSYS;
			goto wait_error;
		}
		__set_current_state(TASK_RUNNING);      /* 将当前任务设置为运行状态 */
	    remove_wait_queue(&dev->r_wait, &wait);    /* 将对应的队列项从等待队列头删除 */

	}

	keyvalue = atomic_read(&dev->keyvalue);
	releasekey = atomic_read(&dev->releasekey);

	if (releasekey) { /* 有按键按下 */	
		if (keyvalue & 0x80) {
			keyvalue &= ~0x80;
			ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
		} else {
			goto data_error;
		}
		atomic_set(&dev->releasekey, 0);/* 按下标志清零 */
	} else {
		goto data_error;
	}
	return 0;

wait_error:
	set_current_state(TASK_RUNNING);		/* 设置任务为运行态 */
	remove_wait_queue(&dev->r_wait, &wait);	/* 将等待队列移除 */
	return ret;

data_error:
	return -EINVAL;

}

static struct file_operations imx6ull_irq_fops {
	.owner = THIS_MODULE,
	.open = imx6uirq_open,
	.read = imx6uirq_read,
};

static int __init imx6ull_init(void)
{
	/* 1、构建设备号 */
	if (imx6u_irq.major)
	{
		imx6u_irq.devid = MKDEV(imx6u_irq.major, 0);
		register_chrdev_region(imx6u_irq.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
	}
	else
	{
		alloc_chrdev_region(&imx6u_irq.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
		imx6u_irq.major = MAJOR(imx6u_irq.devid);
		imx6u_irq.minor = MINOR(imx6u_irq.devid);
	}

	/* 2、注册字符设备 */
	cdev_init(&imx6u_irq.cdev, &imx6ull_irq_fops);
	cdev_add(&imx6u_irq.cdev, imx6u_irq.devid, IMX6UIRQ_CNT);

	/* 3、创建类 */
	imx6u_irq.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
	if (IS_ERR(imx6uirq.class)) {	
		return PTR_ERR(imx6uirq.class);
	}

	/* 4、创建设备 */
	imx6u_irq.device = device_create(imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);
	if (IS_ERR(imx6uirq.device)) {
		return PTR_ERR(imx6uirq.device);
	}

	/* 5、始化按键 */
	atomic_set(&imx6u_irq.key_value, INVAKEY);
	atomic_set(&imx6u_irq.release_value, 0);
	keyio_init();
	return 0;
}

static void __exit imx6ull_exit(void)
{


	cedv_del(&imx6u_irq.cdev);
	unregister_chrdev_region(imx6u_irq.devid, IMX6UIRQ_CNT);
	device_destroy(imx6u_irq.class, imx6u_irq.devid);
	class_destroy(imx6u_irq.class);
}

module_init(imx6ull_init);
module_exit(imx6ull_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("steve");


