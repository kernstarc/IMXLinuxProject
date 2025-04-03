#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define POLLFDS_NUM	11
#define MAX 10

int poll_wait(int listenFd, int connFd[MAX], int connNum)
{
	int i = 0;
	
	if(connNum >= POLLFDS_NUM)
	{
		printf("error connNum=%d\n", connNum);
		return -1;
	}

	struct pollfd pollfds[POLLFDS_NUM];
	pollfds[0].fd = listenFd;
	pollfds[0].events = POLLIN;

	for (i = 0; i < connNum; i++)
	{
		pollfds[i + 1].fd = connFd[i];
		pollfds[i + 1].events = POLLIN;
	}

	int nfds = connNum + 1;
	int ret = poll(pollfds, nfds, 120 * 1000);
	if (ret > 0)	// 2、查询准备就绪描述符
	{
		printf("success, Number of descriptors returned is %d\n", ret);
		for (i = 0; i < nfds; i++)
		{
			if (pollfds[i].revents & POLLIN)
			{
				return pollfds[i].fd;
			}
		}
	}
	else if (ret == 0)
	{
		printf("poll timeout\n");
	}
	
	return -1;
}

// 从connFds数组中删除delFd
int del_connfd(int connFds[10], int connNum, int delFd) 
{
	int tmpFds[10]={0,};
	int i = 0, index = 0;
	
	memcpy(tmpFds, connFds, connNum*sizeof(connFds[0]));
	
	for(i=0; i<connNum; i++)
	{
		if(tmpFds[i] != delFd)
		{
			connFds[index++] = tmpFds[i];
		}
	}
	return index;
}

int main(int argc, char *argv[])
{
	char recvline[256];
	int connFds[MAX];
	int connNum = 0;
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
	{
		perror("socket error" );
		return -1;
	}

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(8080);
	servaddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind error" );
		return -1;
	}

	// 4、监听 listen
	if(listen(sockfd, 10) != 0)
	{
		perror("listen error");
		return -1;
	}

	printf("TcpSer sockfd=%d, start listening\n",sockfd);
	while (1)
	{
		int readableFd = poll_wait(sockfd, connFds, connNum);
		if(readableFd == -1)
		{
			return -1;
		}

		if (readableFd == sockfd)
		{
			printf("listenFd %d is alread to read, calling accept\n", sockfd);
			int connfd = accept(sockfd, NULL, NULL);
			if(connfd < 0)
			{
				perror("accept error" );
				return -1;
			}

			connFds[connNum] = connfd;
			connNum++;
		}
		else
		{
			int connfd = readableFd;
			printf("connfd %d is alread to read\n", connfd);
			int n = read(connfd, recvline, sizeof(recvline));
			if(n>0)
			{
				recvline[n] = 0 ;/*null terminate */
				printf("recv connfd=%d [%s]\n",connfd,recvline);
				write(connfd, "Hello,I am tcp server", strlen("Hello,I am tcp server"));
			}
			printf("close connfd=%d\n",connfd);
			close(connfd);
			connNum = del_connfd(connFds, connNum, connfd);
		}
	}

	return 0;
}


