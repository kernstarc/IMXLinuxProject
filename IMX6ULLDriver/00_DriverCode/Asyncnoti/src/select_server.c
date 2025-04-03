#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/select.h>
#include <sys/time.h>


int select_wait(int listenFd, int connFd[10], int connNum)
{
	int i, maxFd, ret;
	
	// 1、添加感兴趣的描述符到可读描述符集
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(listenFd, &read_set);
	
	for (i = 0; i < connNum, i++)
	{
		FD_SET(connFd[i], &read_set);
	}

	// 2、计算出最大描述符
	maxFd = listenFd;
	for (i = 0; i < connNum; i++)
	{
		if (connFd[i] > maxFd)
		{
			maxFd = connFd[i];
			printf("update max\r\n");
		}
	}
	maxFd++;

	// 3、设置等待时长2分钟
	struct timeval timeout;
	timeout.tv_sec = 2*60;
	timeout.tv_usec= 0;

	// 阻塞等待
	ret = select(maxFd, &read_set, NULL, NULL, &timeout);
	// 即使有多个fd可读，我们也只返回一个描述符，其他的下次处理
	if (ret > 0)
	{
		printf("success, Number of descriptors returned is %d\n", ret);
		if (FD_ISSET(listenFd, &read_set))
		{
			return listenFd;
		}

		for (i = 0; i < connNum; i++)
		{
			if (FD_ISSET(connFd[i], &read_set))
			{
				return connFd[i];
			}
		}
	}
	else if(ret == 0)
	{
		printf("select timeout\n");
	}

	return -1;
}

int del_connfd(int connFds[10], int connNum, int delFd) // 从connFds数组中删除delFd
{
	int tmpFds[10]={0,};
	int i = 0, index = 0;;
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

int main(int argc, char  *argv[])
{
	char recvline[256];
	int connFds[10];
	int connNum = 0;
	
	// 1、创建TCP套接字socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
	{
		perror("socket error" );
		return -1;
	}

	// 2、准备服务端ip和端口
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons (8080);
	// 指定ip地址为 INADDR_ANY，这样要是服务器主机有多个网络接口，服务器进程就可以在任一网络接口上接受客户端的连接
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	
	// 3、绑定 bind
	if (bind(sockfd,(struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
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

	printf("TcpSer sockfd=%d, start listening\n", sockfd);
	while (1)
	{
		int readableFd = select_wait(sockfd, connFds, connNum);
		// 监听描述符可读，使用accept获取客户端套接字
		if (readableFd == sockfd)
		{
			printf("listenFd %d is alread to read, calling accept\n", sockfd);
			int connfd = accept(socket, NULL, NULL);
			if (connfd < 0)
			{
				perror("accept error" );
				return -1;
			}
			connFds[connNum] = connfd;
			connNum++;
		}
		else	// 客户端发数据过来，读取打印
		{
			int connfd = readableFd;
			printf("connfd %d is alread to read\n", connfd);
			int n = read(connfd, recvline, sizeof(recvline));
			if (n > 0)
			{
				recvline[n] = 0;
				printf("recv connfd=%d [%s]\n", connfd, recvline);
				write(connfd, "Hello,I am tcp server", strlen("Hello,I am tcp server"));
			}
			printf("close connfd=%d\n", connfd);
			close(connfd);
			connNum = del_connfd(connFds, connNum, connfd);
		}
	}

	// 7、关闭
	close(sockfd);
	return 0;
}

