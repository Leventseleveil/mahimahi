/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <ctime>

#include "timestamp.hh"
#include "exception.hh"

/**
 * 
 * raw_timestamp 保有时间间隔的结构体，将其“拆分”成秒数和纳秒数。（注意并不是用秒和纳秒来分别表示
 * raw_timestamp 返回（系统当前时间 - 1970年1.1日）的精确到毫秒级时间
 * 原因：timespec 结构体中只有表示秒和纳秒级的，毫秒级的需要两者相加
 * @return uint64_t 
 */
uint64_t raw_timestamp( void )
{
    timespec ts;
    // clock_gettime 获取系统当前时间（从1970年1.1日算起），并将其存储在ts中
    SystemCall( "clock_gettime", clock_gettime( CLOCK_REALTIME, &ts ) );

    // tv_sec 表示（系统当前时间 - 1970年1.1日）小数点前面的秒级的部分
    // tv_nsec 表示（系统当前时间 - 1970年1.1日）小数点后面的纳米级部分
    uint64_t millis = ts.tv_nsec / 1000000; // tv_nsec 表示纳秒  millis 表示毫秒  1毫秒=1000000 纳秒
    millis += uint64_t( ts.tv_sec ) * 1000; // tv_sec 表示秒， 1s = 1000毫秒

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

/**
 * 
 * 
 * @return uint64_t 
 */
uint64_t timestamp( void )
{
    return raw_timestamp() - initial_timestamp();
}
