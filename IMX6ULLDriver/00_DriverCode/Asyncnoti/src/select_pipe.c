#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>


	
int main(int argc, char *argv[])
{
	fd_set readfds;
	struct timeval timeout;
	int ret, fd_max;

	// 创建两个管道
    int pipefds1[2];
    int pipefds2[2];
    pipe(pipefds1);
    pipe(pipefds2);
	
	// 向管道写入数据
    write(pipefds1[1], "Hello", 5);
    //write(pipefds2[1], "World", 5);

	while (1) 
	{
		FD_ZERO(&readfds);
		FD_SET(pipefds1[0], &readfds);
		FD_SET(pipefds2[0], &readfds);	
		FD_SET(STDIN_FILENO, &readfds);  // 标准输入

		// 设置最大文件描述符
		fd_max = (pipefds1[0] > pipefds2[0]) ? pipefds1[0] : pipefds2[0];
		printf("[%d, %d]fd max : %d\r\n", pipefds1[0], pipefds2[0], fd_max);

		// 设置超时时间
		timeout.tv_sec = 5;
        timeout.tv_usec = 0;

		ret = select(fd_max + 1, &readfds, NULL, NULL, &timeout);
		if (ret == -1)
		{
			perror("select");
            exit(EXIT_FAILURE);
		}
		else if (ret == 0)
		{
			printf("Timeout!\n");
            break;
		}
		else
		{
			if (FD_ISSET(STDIN_FILENO, &readfds)) 
			{
	            //char buffer[256];
	            //read(STDIN_FILENO, buffer, sizeof(buffer));
	            printf("Input: ............. \r\n");
            }

			if (FD_ISSET(pipefds1[0], &readfds))
			{
				char buf[6];
				read(pipefds1[0], buf, 5);
				buf[5] = '\0';
				printf("Data from pipe1: %s\n", buf);
			}
			
			if (FD_ISSET(pipefds2[0], &readfds)) {
                char buf[6];
                read(pipefds2[0], buf, 5);
                buf[5] = '\0';
                printf("Data from pipe2: %s\n", buf);
            }
			break;
		}

	}

	close(pipefds1[0]);
    close(pipefds1[1]);
    close(pipefds2[0]);
    close(pipefds2[1]);

	return 0;
}

