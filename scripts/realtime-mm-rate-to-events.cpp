#include <iostream>//必写，其实就是c的#include <stdio.h>
#include <sys/un.h>
#include <unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
using namespace std;

int main() {
    int ms = 0; // 现实时间 单位ms
    int time = 0; // my是私有变量， 标量 $ 开始
    int reserve_bits = 0; // 缓冲的bits
    int PACKET_LENGTH = 12000; // bits = 1500B
    int kilobits_this_millisecond = 0;

    while (1){
        kilobits_this_millisecond = rand() % 36 + 1;
        // cout<<kilobits_this_millisecond<<endl;
        reserve_bits += 1000 * kilobits_this_millisecond;
        while (reserve_bits >= PACKET_LENGTH){
            cout<<time<<endl;
            reserve_bits -= PACKET_LENGTH;
        }
        
        time++;
        sleep(1);
    }
    

    return 0;
}