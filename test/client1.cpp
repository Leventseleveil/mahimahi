#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


int main()
{
	char send_msg[1024];
	char recv_msg[1024];
	int sockfd;
	int ret;
	int address_len,result;
	struct sockaddr_un address;

	// 1. 创建socket
	if(-1 == (sockfd = socket(AF_UNIX, SOCK_STREAM, 0))){
		perror("socket");
		return -1;
	}

	// 2.初始化sockaddr_un address 结构体信息，并且连接服务器
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "./process_communication"); // strcpy(addr.sun_path, "socket_file");   //与服务器一致
	address_len = sizeof(address);
	result = connect(sockfd, (struct sockaddr *)&address, address_len);
	if(result == -1){
		// 返回非0说明连接失败
		if(0 != access("./process_communication", F_OK))
		{
			printf("[Warning]: Wait for Server socket Creating!!!\n");
			usleep(500 * 1000);
			
			if(-1 == connect(sockfd, (struct sockaddr *)&address,address_len)){
				perror("connect");
				return -1;
			}
		}else{
			perror("connect");
			return -1;
		}
	}
	// 开始交流
	while(1)
	{	// 这段代码有问题 得先写再读
		memset(send_msg, 0, sizeof(send_msg));
		scanf("%s",send_msg); // 只能从这里输入
		ret = write(sockfd,send_msg,sizeof(send_msg));
		if (ret <= 0)
		{
			perror("send\n");
		}

		// 此段可删
		// ret = read(sockfd,recv_msg, sizeof(recv_msg));
		// if (ret <= 0)	
		// {
		// 	perror("read");
		// 	exit(-1);
		// }
		// printf("recv msg is %s\n",recv_msg);
	}
	return 0;
}

