/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <ctime>

#include "timestamp.hh"
#include "exception.hh"
#include <unistd.h>


uint64_t raw_timestamp( void )
{
    timespec ts;
    // clock_gettime 获取系统当前时间（从1970年1.1日算起），并将其存储在ts中
    SystemCall( "clock_gettime", clock_gettime( CLOCK_REALTIME, &ts ) );

    // tv_sec 表示（系统当前时间 - 1970年1.1日）小数点前面的秒的部分
    // tv_nsec 表示（系统当前时间 - 1970年1.1日）小数点后面的纳米级部分
    uint64_t millis = ts.tv_nsec / 1000000; // tv_nsec 表示纳秒  millis 表示毫秒  1毫秒=1000000 纳秒
    millis += uint64_t( ts.tv_sec ) * 1000;

    return millis;
}

/**
 * static 随着第一次函数的调用而初始化，却不随着函数的调用结束而销毁
 * 第一次调用的时候初始化，且只会初始化一次，也就是第二次调用的时候不会继续初始化，而会直接跳过。
 * @return uint64_t 
 */
uint64_t initial_timestamp( void )
{
    static uint64_t initial_value = raw_timestamp();
    return initial_value;
}

uint64_t timestamp( void )
{
    return raw_timestamp() - initial_timestamp();
}
static int i = 1;

uint64_t foo() {
    i++;
    return i;
}

uint64_t f() {
    static int i = 1; // note:1
        //int i = 1;  // note:2
    i++;
    return i;
}



int main() {
    raw_timestamp();
    // std::cout<<"foo1(): "<<foo()<<std::endl;
    // std::cout<<"foo2(): "<<foo()<<std::endl;
    // std::cout<<"foo3(): "<<foo()<<std::endl;
    // std::cout<<"f1(): "<<f()<<std::endl;
    // std::cout<<"f2(): "<<f()<<std::endl;
    // std::cout<<"f3(): "<<f()<<std::endl;
    // // std::cout<<"initial_timestamp1(): "<<initial_timestamp()<<std::endl;
    // std::cout<<"timestamp(): "<<timestamp()<<std::endl;
    // sleep(1);
    // std::cout<<"raw_timestamp(): "<<raw_timestamp()<<std::endl;
    // std::cout<<"initial_timestamp2(): "<<initial_timestamp()<<std::endl; // static 随着第一次函数的调用而初始化，却不随着函数的调用结束而销毁
    // std::cout<<"timestamp(): "<<timestamp()<<std::endl;
    // sleep(1);
    // std::cout<<"timestamp(): "<<timestamp()<<std::endl;
}