#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>

#define MAX 10

// 成功返回监听套接字， 出错返回-1
int sockfd_init(void)
{
	int sockfd, ret;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("socket");
		return -1;
	}

	struct sockaddr_in seraddr; 
	seraddr.sin_family = AF_INET;
	seraddr.sin_port = htons(8888);
	inet_pton(AF_INET, "127.0.0.1", &seraddr.sin_addr.s_addr );

	ret = bind(sockfd, (struct sockaddr*)&seraddr, sizeof(struct sockaddr_in));
	if(ret < 0){
		perror("bind");
		return -1;
	}

	//通知 内核监听
	ret = listen(sockfd, MAX);
	if(ret < 0){
		perror("listen");
		return -1;
	}
	 
	return sockfd;
}


int main(int argc, char *argv[])
{
	int sockfd;
	int ret, cfd, efd;

	int count;
	char buff[1024] = "";

	efd = epoll_create(MAX);
	if (efd < 0)
	{
		perror("epoll_create");
		return -1;
	]

	//创建监听套接字
    sockfd = sockfd_init();
    if(sockfd < 0)
	{
		return -1;
	}

	// 将sockfd加入到集合中
	struct epoll_event ev, evs[MAX];
	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &ev);
	
	while (1)
	{
		//监听集合中所有文件描述符
		printf("wait...\n");
		count = epoll_wait(efd, evs, MAX, -1);
		printf("wait over...\n");
        if(count < 0){
			perror("epoll_wait");
			break;
		}
		
		for(int i = 0; i < count; i++)
		{
			int tfd = evs[i].data.fd;
			if (tfd == sockfd)	// 接收客户端
			{
				//1、接收客户端
				printf("accept..\n");
				cfd = accept(sockfd, NULL, NULL);
				if (cfd < 0)
				{
				    perror("accept");
				    continue;
			    }
				printf("accept  over..\n");
				//2、将cfd加入集合
				ev.data.fd = cfd;
				epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);
			}
			else 	// 已经建立连接的客户端发来数据
			{
				printf("read..\n");
				ret = read(tfd, buff, 1024);
				printf("read  over..\n");

				if(ret < 0){
					perror("read");
					close(tfd);
					epoll_ctl(efd, EPOLL_CTL_DEL, tfd, NULL);
					continue;
				}
				else if(0 == ret)
				{
					printf("TCP  broken...\n");
					close(tfd);
					epoll_ctl(efd, EPOLL_CTL_DEL, tfd, NULL);
					continue;
				}
                buff[ret] = '\0';
				printf("buff: %s\n", buff);
			}
		}
	}

	return 0;
}


