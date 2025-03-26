#!/bin/bash

# 1.需要先修改Makefile文件中的编译器路径和架构类型
# CROSS_COMPILE ?= /home/starc/share/work/imx6ull-code/CrossCompiler/bin/arm-linux-gnueabihf-
# ARCH ?= arm

# 2.修改中文字符的支持

# 3.配置选项，生成.config文件
#make defconfig		# 默认选择配置选项，其他 allyesconfig,allnoconfig
#make menuconfig	# 手动图形化配置选项

# 4.编译BusyBox,指定编译结果存放路径
make -j2
make install CONFIG_PREFIX=/home/starc/share/work/imx6ull-code/BusyBoxCode/build



