#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>


char userData[] = {"user data!"};

int main(int argc, char *argv[])
{

	int fd;
	int ret;
	char *fileName;
	char readbuf[100], writebuf[100];

	if (argc != 3)
	{
		printf("Error Usage!\r\n");
		return -1;
	}

	fileName = argv[1];

	fd = open(fileName, O_RDWR);
	if (fd < 0) {
		perror("error open");
		return -1;
	}

	if (atoi(argv[2]) == 1)
	{
		ret = read(fd, readbuf, sizeof(readbuf));
		if (ret < 0)
		{
			perror("error read");
			return -1;
		}
		else
		{
			printf("read:%s\r\n", readbuf);
		}
	}

	if (atoi(argv[2]) == 2)
	{
		memcpy(writebuf, userData, sizeof(userData));
		ret = write(fd, writebuf, sizeof(writebuf));
		if (ret < 0)
		{
			perror("error write");
			return -1;
		}
		else
		{
			printf("write:%s\r\n", writebuf);
		}
	}

	ret = close(fd);
	if (ret < 0)
	{
		perror("error close");
		return -1;
	}

	return 0;
}


