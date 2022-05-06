/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <limits>
#include <cassert>

#include "link_queue.hh"
#include "timestamp.hh"
#include "util.hh"
#include "ezio.hh"
#include "abstract_packet_queue.hh"

using namespace std;

// 构造函数及初始化过程
LinkQueue::LinkQueue( const string & link_name, const string & filename, const string & logfile,
                      const bool repeat, const bool graph_throughput, const bool graph_delay,
                      unique_ptr<AbstractPacketQueue> && packet_queue,
                      const string & command_line )
    : next_delivery_( 0 ), // next_delivery_为无符号整数，初始化为0，*每传递一次+1
      schedule_(), // schedule 表示时刻表容器/时间戳容器
      base_timestamp_( timestamp() ), // base_timestamp_ 表示基准时间 初始值为0（之后不重复的话一直为0）， base_timestamp_一般为0，重复的话为0 + traces文件最后一行 * 重复次数， 
      packet_queue_( move( packet_queue ) ), // move 将指针的所有权从一个unique_ptr转移给另一个unique_ptr
      packet_in_transit_( "", 0 ),
      packet_in_transit_bytes_left_( 0 ),
      output_queue_(),
      log_(),
      throughput_graph_( nullptr ),
      delay_graph_( nullptr ),
      repeat_( repeat ), // 初始值为true，表示默认重复读取文件
      finished_( false ) // finished表示读取完成的信号，初始当然未完成
{
    assert_not_root(); // 断言不是root用户

    /* open filename and load schedule */
    ifstream trace_file( filename ); // ifstream是从硬盘到内存，其实所谓的流缓冲就是内存空间

    // 检查
    if ( not trace_file.good() ) { // good()读写正常
        throw runtime_error( filename + ": error opening for reading" );
    }

    string line;

    // 这里是构造函数初始化，所以这段要删除
    while ( trace_file.good() and getline( trace_file, line ) ) {
        /**
         * 
         * getline(cin, inputLine);
         * 其中 cin 是正在读取的输入流，而 inputLine 是接收输入字符串的 string 变量的名称。下面的程序演示了 getline 函数的应用：
         * 
         */

        // 首先检查该行是否为空
        if ( line.empty() ) {
            throw runtime_error( filename + ": invalid empty line" );
        }

        /**
         * 在traces文件中的每一行代表着一个包发送机会：在这个时间点MTU大小的包将被发送。
         * ms 表示时间戳, 每一ms发送一个包
         * myatoi 将字符串转换为int
         */
        const uint64_t ms = myatoi( line ); 

        // 检查每行是否单调递增，因为时间戳必须是单调不变的
        if ( not schedule_.empty() ) { // 不用检查第一个，因为就一个数无法检查是否单调递增（用empty的原因）
            if ( ms < schedule_.back() ) { // back 用于访问矢量的最后一个元素，它返回对矢量的最后一个元素的引用。
                throw runtime_error( filename + ": timestamps must be monotonically nondecreasing" ); // 时间戳必须是单调不变的
            }
        }

        /**
         * emplace_back() 在实现时，直接在容器尾部创建这个元素，省去了拷贝或移动元素的过程。（对比push_back）
         */
        schedule_.emplace_back( ms ); // *schedule_是个64位机器上的vector
    }

    if ( schedule_.empty() ) {
        throw runtime_error( filename + ": no valid timestamps found" );
    }

    if ( schedule_.back() == 0 ) { // 如果vector的元素类型是int，默认初始化为0；
        throw runtime_error( filename + ": trace must last for a nonzero amount of time" ); // trace必须持续一段时间
    }

    /* open logfile if called for */ // 如果需要的话，打开日志文件
    if ( not logfile.empty() ) {
        log_.reset( new ofstream( logfile ) );
        if ( not log_->good() ) {
            throw runtime_error( logfile + ": error opening for writing" );
        }

        *log_ << "# mahimahi mm-link (" << link_name << ") [" << filename << "] > " << logfile << endl;
        *log_ << "# command line: " << command_line << endl;
        *log_ << "# queue: " << packet_queue_->to_string() << endl;
        *log_ << "# init timestamp: " << initial_timestamp() << endl;
        *log_ << "# base timestamp: " << base_timestamp_ << endl;
        const char * prefix = getenv( "MAHIMAHI_SHELL_PREFIX" );
        if ( prefix ) {
            *log_ << "# mahimahi config: " << prefix << endl;
        }
    }

    /* create graphs if called for */  // 如果需要的话创建图表
    // 对应linkshell.cc 中的meter-uplink/downlink 等参数是否打开
    if ( graph_throughput ) {
        // 重制图表
        throughput_graph_.reset( new BinnedLiveGraph( link_name + " [" + filename + "]",
                                                      { make_tuple( 1.0, 0.0, 0.0, 0.25, true ),
                                                        make_tuple( 0.0, 0.0, 0.4, 1.0, false ),
                                                        make_tuple( 1.0, 0.0, 0.0, 0.5, false ) },
                                                      "throughput (Mbps)",
                                                      8.0 / 1000000.0,
                                                      true,
                                                      500,
                                                      [] ( int, int & x ) { x = 0; } ) );
    }

    // 对应linkshell.cc 中的meter-uplink/downlink-delay是否打开
    if ( graph_delay ) {
        delay_graph_.reset( new BinnedLiveGraph( link_name + " delay [" + filename + "]",
                                                 { make_tuple( 0.0, 0.25, 0.0, 1.0, false ) },
                                                 "queueing delay (ms)",
                                                 1, false, 250,
                                                 [] ( int, int & x ) { x = -1; } ) );
    }
}

// 记录到达
void LinkQueue::record_arrival( const uint64_t arrival_time, const size_t pkt_size )
{
    /* log it */
    if ( log_ ) {
        *log_ << arrival_time << " + " << pkt_size << endl; // *log_ *表示挥动魔法棒
    }

    /* meter it */
    if ( throughput_graph_ ) {
        throughput_graph_->add_value_now( 1, pkt_size );
    }
}

// 记录丢弃
void LinkQueue::record_drop( const uint64_t time, const size_t pkts_dropped, const size_t bytes_dropped)
{
    /* log it */
    if ( log_ ) {
        *log_ << time << " d " << pkts_dropped << " " << bytes_dropped << endl;
    }
}

// 记录离开的时机
void LinkQueue::record_departure_opportunity( void )
{
    /* log the delivery opportunity */
    if ( log_ ) {
        *log_ << next_delivery_time() << " # " << PACKET_SIZE << endl;
    }

    /* meter the delivery opportunity */
    /** 每行表示传送一个包，图上折线的变化其实都是同一个时间戳堆叠出来的
     *  7
     *  8
     *  8
     *  8
     *  8
     *  8
     *  8
     */
    if ( throughput_graph_ ) {
        throughput_graph_->add_value_now( 0, PACKET_SIZE );
    }    
}

// 记录离开
void LinkQueue::record_departure( const uint64_t departure_time, const QueuedPacket & packet )
{
    /* log the delivery */
    if ( log_ ) {
        *log_ << departure_time << " - " << packet.contents.size()
              << " " << departure_time - packet.arrival_time << endl;
    }

    /* meter the delivery */
    if ( throughput_graph_ ) {
        throughput_graph_->add_value_now( 2, packet.contents.size() );
    }

    if ( delay_graph_ ) {
        delay_graph_->set_max_value_now( 0, departure_time - packet.arrival_time );
    }    
}

// 读包
void LinkQueue::read_packet( const string & contents )
{
    const uint64_t now = timestamp(); // 当前时间戳，从函数开始运行后经过的毫秒数， 应该是表示图中的横坐标

    if ( contents.size() > PACKET_SIZE ) { // 如果传送的包容量超过限定 const static unsigned int PACKET_SIZE = 1504;
        throw runtime_error( "packet size is greater than maximum" );
    }

    rationalize( now );

    record_arrival( now, contents.size() );

    unsigned int bytes_before = packet_queue_->size_bytes();
    unsigned int packets_before = packet_queue_->size_packets();

    packet_queue_->enqueue( QueuedPacket( contents, now ) );

    assert( packet_queue_->size_packets() <= packets_before + 1 );
    assert( packet_queue_->size_bytes() <= bytes_before + contents.size() );
    
    unsigned int missing_packets = packets_before + 1 - packet_queue_->size_packets();
    unsigned int missing_bytes = bytes_before + contents.size() - packet_queue_->size_bytes();
    if ( missing_packets > 0 || missing_bytes > 0 ) {
        record_drop( now, missing_packets, missing_bytes );
    }
}

// next_delivery_time 返回下一次传递时间
/**
 * 最重要的一个函数
 * 
 * @return 相当于trace中的一行
 */
uint64_t LinkQueue::next_delivery_time( void ) const
{
    if ( finished_ ) { // 如果完成了，返回-1 实时发送的话不考虑，咱可以直接exit
        return -1;
    } else {
        /**
         * at()是std::vector的一个方法，根据元号返回元值。
         * 相比于[]运算符，这方法会进行边界检查，如果越界则引发异常，而[]则不会。
         * 因此本方法相对来讲安全性高，但效率低（当然这种程度一般可以无视）。
         */
        //下一次传递时间 = 下一次传送的次数对应的时间戳（如果比如连着几行8的话，虽然next_delivery_不断+1，但是其对应的时间戳还是不变的） + 基准时间
        // .at(idx) 传回索引idx所指的数据，如果idx越界，抛出out_of_range。
        return schedule_.at( next_delivery_ ) + base_timestamp_; // base_timestamp_
    }
}


/**
 * use_a_delivery_opportunity 的作用是给next_delivery_+1,
 * 如果到达traces文件的头并且repeat_参数是false的话，则结束整个程序
 * 
 */
void LinkQueue::use_a_delivery_opportunity( void )
{
    record_departure_opportunity(); // 在日志和图表上记录

    // 重复用的，如果当前循环完了，就从第一行重新开始
    next_delivery_ = (next_delivery_ + 1) % schedule_.size();

    /* wraparound 环绕的 */
    if ( next_delivery_ == 0 ) { // 如果next_delivery_已经到traces文件的头了
        if ( repeat_ ) { // 如果重复的话继续下一轮
            // base_timestamp_ =base_timestamp_ + schedule_.back()
            // base_timestamp_变为上一轮的基准时间+时间戳容器的最后一个时间戳
            // 另：base_timestamp_只在该行有变化
            base_timestamp_ += schedule_.back(); // back()函数返回当前vector最末一个元素的引用
        } else { // 否则便完成整个程序
            finished_ = true;
        }
    }
}

/* emulate the link up to the given timestamp */ // 仿真链接直到给定的时间戳
/* this function should be called before enqueueing any packets and before
   calculating the wait_time until the next event */
// 在将任何数据包排入队列之前，以及在计算下一个事件之前的等待时间之前，应该调用此函数
void LinkQueue::rationalize( const uint64_t now )
{
    while ( next_delivery_time() <= now ) {
        const uint64_t this_delivery_time = next_delivery_time();

        /* burn a delivery opportunity */
        unsigned int bytes_left_in_this_delivery = PACKET_SIZE;
        use_a_delivery_opportunity();

        while ( bytes_left_in_this_delivery > 0 ) {
            if ( not packet_in_transit_bytes_left_ ) {
                if ( packet_queue_->empty() ) {
                    break;
                }
                packet_in_transit_ = packet_queue_->dequeue();
                packet_in_transit_bytes_left_ = packet_in_transit_.contents.size();
            }

            assert( packet_in_transit_.arrival_time <= this_delivery_time );
            assert( packet_in_transit_bytes_left_ <= PACKET_SIZE );
            assert( packet_in_transit_bytes_left_ > 0 );
            assert( packet_in_transit_bytes_left_ <= packet_in_transit_.contents.size() );

            /* how many bytes of the delivery opportunity can we use? 我们可以使用多少字节的传递机会 */
            const unsigned int amount_to_send = min( bytes_left_in_this_delivery,
                                                     packet_in_transit_bytes_left_ );

            /* send that many bytes 发送多少字节*/
            packet_in_transit_bytes_left_ -= amount_to_send;
            bytes_left_in_this_delivery -= amount_to_send;

            /* has the packet been fully sent? 数据包已经全部寄出了吗？ */
            if ( packet_in_transit_bytes_left_ == 0 ) {
                record_departure( this_delivery_time, packet_in_transit_ );

                /* this packet is ready to go */
                output_queue_.push( move( packet_in_transit_.contents ) );
            }
        }
    }
}

void LinkQueue::write_packets( FileDescriptor & fd )
{
    while ( not output_queue_.empty() ) {
        fd.write( output_queue_.front() );
        output_queue_.pop();
    }
}

/**
 * 等待的时间
 * 
 * @return 等待的时间（无需等待即刻出发 为0 或是 等待next_delivery_time() - now 毫秒）
 */
unsigned int LinkQueue::wait_time( void )
{
    const auto now = timestamp();

    rationalize( now ); // 如果 next_delivery_time() <= now， 则相当于该函数啥没干

    /**
     * 
     * next_delivery_time() 相当于返回traces中的下一行，也就是下一次包传递时间
     * traces文件中可能为：
     * 1056
     * 1143
     * 1210
     * ...
     * 
     * 也就是可能在1056ms才开始发送，
     * 而当前的timestamp()可能才刚开始 为9
     * 此时就需要等待
     */
    if ( next_delivery_time() <= now ) {
        return 0;
    } else {
        return next_delivery_time() - now;
    }
}

bool LinkQueue::pending_output( void ) const
{
    return not output_queue_.empty();
}
