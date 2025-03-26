#!/bin/sh

input_type="$1"
driver_type="d"
driver_done_type="dd"
dts_type="s"
dtb_type="b"
dts_done_type="sd"


echo "input_type:$input_type"
echo "driver_type:$driver_type"
echo "driver_done_type:$driver_done_type"
echo "dts_type:$dts_type"
echo "dtb_type:$dtb_type"
echo "dts_done_type:$dts_done_type"


if [ $input_type = $driver_type ]; then		# 驱动代码
	echo Start to update Driver file
	scp -r ./DriverCode/* steve@192.168.3.57:/home/steve/imx6ull-code/DriverCode
elif [ $input_type = $driver_done_type ]; then		# 编译完成的驱动代码
	echo Start to update Driver Done file
	scp -r ./DriverCode/* root@192.168.3.40:/home/work/imx6ull-code/DriverCode
elif [ $input_type = $dts_type ]; then		# 设备树源文件
	echo Start to update DTS file
	scp -r ./KernelCode/arch/arm/boot/dts/imx6ull-alientek-emmc.dts steve@192.168.3.57:/home/steve/imx6ull-code/KernelCode/arch/arm/boot/dts
elif [ $input_type = $dts_done_type ]; then	# 编译完成的DTS到TFTP
	echo Start to update DTS Done file
	rm ../tftp/imx6ull-14x14-emmc-7-1024x600-c.dtb
	mv ../tftp/imx6ull-alientek-emmc.dtb ../tftp/imx6ull-14x14-emmc-7-1024x600-c.dtb
elif [ $input_type = $dtb_type ]; then		# 发送DTB
	echo Start to update DTB file
	scp -r ./KernelCode/arch/arm/boot/dts/imx6ull-alientek-emmc.dtb root@192.168.3.40:/home/work/tftp
else
	other_type="$1"
	echo "Start to update $other_type file"
	scp $other_type steve@192.168.3.57:/home/steve/imx6ull-code/$2
	

fi
















