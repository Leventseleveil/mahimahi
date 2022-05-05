#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <thread>
#include <iostream>//必写，其实就是c的#include <stdio.h>
using namespace std;//必写，声明使用std名称空间，不用理解

// g++ thread1.cpp -o thread1 -lpthread
void procServer(string filename)
{
    char sun_path[50] = "./process_communication_";
    char temp[50];
    strcpy(temp, filename.c_str());
    strcat(sun_path, temp);
    cout<<sun_path<<endl;

    cout << "我是子线程,传入参数为" << filename << endl;
    cout << "子线程中显示子线程id为" << this_thread::get_id()<< endl;

    char buf[1024];
	int msg_len;
	int server_fd,client_fd;
	int server_len;
	socklen_t client_len;

	// 与网络编程不一样的地方是服务器端bind的时候用的是sockaddr_un结构，客户端connect的时候用的也是sockaddr_un结构，而不是sockaddr_in或sockaddr。
	struct sockaddr_un server_address;
	struct sockaddr_un client_address;
	
    // 创建socket的本地进程间通信过程
	server_fd = socket(AF_UNIX, SOCK_STREAM, 0); // AF_UNIX/AF_LOCAL   Local communication(本地,进程间通信)

	// 给 server_address 添加属性
	server_address.sun_family = AF_UNIX;
	// sockaddr_un.sun_path的值是bind()函数生成的socket类型文件的路径，并且客户端与服务器端的这个sockaddr_un结构的sun_path是一致的
	// char *strcpy(char *dest, const char *src)
	strcpy(server_address.sun_path, sun_path);
	unlink(sun_path); // unlinl 删除指定文件
	server_len = sizeof(server_address);
	client_len = sizeof(client_address);

	bind(server_fd, (struct sockaddr *)&server_address, server_len);
	listen(server_fd, 5);
	client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_len);

	
	while(1)
	{
		if(-1 == (msg_len = read(client_fd, buf, sizeof(buf))))
		{
			perror("read");
			exit(-1);
		}
		cout<<buf<<endl;
	}
	close(server_fd);
	close(client_fd);
}
int main()
{
    cout << "我是主线程" << endl;
    string filename = "uplink";
    thread th(procServer, ref(filename));//第一个参数为函数名，第二个参数为该函数的第一个参数，如果该函数接收多个参数就依次写在后面。此时线程开始执行。
    th.detach();

    sleep(100);
    return 0;
}