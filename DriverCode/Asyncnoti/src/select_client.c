#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>



int main(int argc, char  *argv[])
{
	char recvline[256];
	int n = 0;
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
	servaddr.sin_port = htons(8080);
	// 设置本机IP为服务端IP
	if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0) 
		perror("inet_pton error");

	// 3、连接 connect
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect error");
		return -1;
	}

	// 4、交换数据
	printf("TcpCli sockfd=%d, send msg\n", sockfd);
	write(sockfd, "Hello,I am tcp client", strlen("Hello,I am tcp client"));
	
	while ((n = read (sockfd, recvline, sizeof(recvline))) > 0)
	{
		recvline[n] = 0 ;	/*null terminate */
		printf("recv[%s]\n", recvline);
	}

	if (n < 0)
		perror("read error" );
	
	// 5、关闭
	printf("close fd %d\n",sockfd);
	close(sockfd);

	return 0;
}


