# 需要的系统内核路径
KERNELDIR := /home/steve/imx6ull-code/KernelCode	
# 现在的路径	
CURRENT_PATH := $(shell pwd)
# 需要编译生成的模块名字
MODULE_NAME := led
# 编译时需要的文件，会自动推导出需要的.c文件
obj-m := beep.o
# 多文件编译时设置
# hello-objs := hello1.o hello2.o 
 
# 编译目标
build: kernel_modules
kernel_modules:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) modules     
 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(CURRENT_PATH) clean
 
