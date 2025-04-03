#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"

#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <linux/input.h>


/*
 * The event structure itself
struct input_event {
	struct timeval time;
	__u16 type;
	__u16 code;
	__s32 value;
};
*/
static struct input_event inputevent;


/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd;
	int err = 0;
	char *filename;

	filename = argv[1];
	if (argc != 2)
	{
		printf("Error Usage!\r\n");
		return -1;
	}

	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		perror("Error open");
		return -1;
	}
	
	while (1)
	{
		err = read(fd, &inputevent, sizeof(inputevent));
		if (err > 0)	// 读取数据成功
		{
			switch (inputevent.type)
			{
				case EV_KEY:
					if (inputevent.code < BTN_MISC)
					{
						printf("key %d %s\r\n", 
							inputevent.code, inputevent.value ? "press" : "release");
					}
					else 
					{
						printf("button %d %s\r\n", 
							inputevent.code, inputevent.value ? "press" : "release");
					}
					break;
				default:
					printf("Other EV Value\r\n");
					break;
			}
		} 
		else 
		{
			printf("读取数据失败\r\n");
		}
	}
	return 0;
}



