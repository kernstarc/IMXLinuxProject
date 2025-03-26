
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
 
// 信号处理函数
void  handle(int sig){
	printf("resv sig: %d\n", sig);
}
 
int main(){
	int sockfd, ret;
	struct sockaddr_in seraddr; 
	char buff[1024];
 
	signal(SIGPIPE,  handle);
 
	// 创建监听套接字
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("socket");
		return -1;
	}
 
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(8888);
	inet_pton(AF_INET, "127.0.0.1", &seraddr.sin_addr.s_addr );
	// 连接服务端
	ret = connect(sockfd, (struct sockaddr*)&seraddr, sizeof(struct sockaddr_in));
	if(ret < 0){
		perror("connnect");
		return -1;
	}
 
	// 发送请求
	while(1){
		printf("请输入数据:");
        fflush(stdin);
		scanf("%s", buff);
		ret = write(sockfd, buff, strlen(buff));
		if(ret < 0){
			perror("write");
			return -1;
		}
	}
 
	// 关闭套接字
	close(sockfd);
 
	return 0;
}


























