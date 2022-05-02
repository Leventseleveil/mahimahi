#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iostream>//必写，其实就是c的#include <stdio.h>
#include <bits/stdc++.h>//万能头文件，强烈建议必写
using namespace std;//必写，声明使用std名称空间，不用理解

// char* int2chars(int time, char* chars) {
// 	int i = 0;
// 	while (time) {
// 		chars[i++] = time % 10 + '0';
// 		time /= 10;
// 	}
// }


int main() {
	int ms = 0; // 现实时间 单位ms
    int time = 0; // my是私有变量， 标量 $ 开始
    int reserve_bits = 0; // 缓冲的bits
    int PACKET_LENGTH = 12000; // bits = 1500B
    int kilobits_this_millisecond = 0;
	string a;

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
	while(1){	// 这段代码有问题 得先写再读
		// //scanf("%s",send_msg); // 只能从这里输入
		// cin>>send_msg;

		kilobits_this_millisecond = rand() % 36 + 1;
        // cout<<kilobits_this_millisecond<<endl;
        reserve_bits += 1000 * kilobits_this_millisecond;
        while (reserve_bits >= PACKET_LENGTH){
			memset(send_msg, 0, sizeof(send_msg));
            a = to_string(time);
			strcpy(send_msg, a.c_str());
			ret = write(sockfd, send_msg, sizeof(send_msg));
			if (ret <= 0) {perror("send\n");}
            reserve_bits -= PACKET_LENGTH;
        }
        
        time++;
        sleep(1);

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

