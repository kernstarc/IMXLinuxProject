# IMXLinuxProject

## 文件说明
CrossCompiler：交叉编译器
BusyBoxCode：BusyBox源码
U-BootCode：uboot源码
KernelCode：内核源码
CV1813HDriver：CV1813H驱动代码
IMX6ULLDriver：i.MX6ULL驱动代码


## 学习目标

### BootLoader

#### 编译-顶层Makefile-文件夹说明
	- [x] uboot顶层Makefile注释，了解其主要的编译流程
	- [x] 了解编译生成的各种uboot.bin等文件
	- [x] 了解各文件夹作用
	- [x] 编写build.sh脚本编译uboot源码
	
#### 启动流程
	- [x] 通过uboot.lds连接脚本了解uboot启动流程
	- [x] 了解bootz启动linux内核流程
	
#### 移植
	- [x] 添加自定义开发板

#### 图形化配置
	- [x] 了解menuconfig\Kbuild\Kconfig，添加自定义菜单
	- [x] 了解uboot添加自定义命令的流程
	- [] 自定义开机LOGO

#### 相关命令	
	- [] bootz
	- [] 网络启动相关
	


	
### Rootfs
	- [x] BusyBox编译
	- [x] BusyBox构建根文件系统
	- [x] BusyBox移植需要的库文件
	- [] buildroot
	- [] yocto
	




### Kernel
	- [x] 顶层Makefile注释，了解其主要的编译流程
	- [x] 编写build.sh脚本编译源码
	- [] 连接脚本vxlinux.lds
	- [] 内核启动流程
	- [] 了解各种头文件的作用





### Driver

#### imx6ull
	- [x] 编译
	- [x] 
	- [x] 
	- [x] 

#### cv1813h
	- [x] 编译
	- [x] 
	- [x] 
	- [x] 

