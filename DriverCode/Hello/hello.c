#include <linux/init.h>  
#include <linux/kernel.h>  
#include <linux/module.h>  
 
static int hello_init(void)  
{  
    printk("load module\n");  
    return 0;  
}  
 
static void hello_exit(void)  
{  
    printk("unload module\n");  
    return;  
}  
 
module_init(hello_init);  
module_exit(hello_exit);  
MODULE_LICENSE("GPL");  
MODULE_AUTHOR("steve");

