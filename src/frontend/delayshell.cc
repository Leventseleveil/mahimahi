/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <vector>
#include <string>

#include "delay_queue.hh"
#include "util.hh"
#include "ezio.hh"
#include "packetshell.cc"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        /* clear environment while running as root */
        // 以root用户身份运行时清除环境
        char ** const user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        // argc 为程序运行时发送给main函数的命令行参数的个数
        // argv[ 0 ] 为 mm-delay（argv[0]指向程序运行的全路径名 ）
        if ( argc < 2 ) { 
            throw runtime_error( "Usage: " + string( argv[ 0 ] ) + " delay-milliseconds [command...]" );
        }

        const uint64_t delay_ms = myatoi( argv[ 1 ] );

        vector< string > command;

        if ( argc == 2 ) {
            command.push_back( shell_path() );
        } else {
            for ( int i = 2; i < argc; i++ ) {
                command.push_back( argv[ i ] );
            }
        }

        // 重新建立一个shell，形如：[delay 50 ms] parallels@ubuntu-linux-20-04-desktop:~$ 
        PacketShell<DelayQueue> delay_shell_app( "delay", user_environment );

        delay_shell_app.start_uplink( "[delay " + to_string( delay_ms ) + " ms] ",
                                      command,
                                      delay_ms );
        delay_shell_app.start_downlink( delay_ms );
        return delay_shell_app.wait_for_exit();
    } catch ( const exception & e ) {
        print_exception( e );
        return EXIT_FAILURE;
    }
}
