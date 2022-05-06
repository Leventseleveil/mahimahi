/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef QUEUED_PACKET_HH
#define QUEUED_PACKET_HH

#include <string>

struct QueuedPacket
{
    uint64_t arrival_time; // 到达时间
    std::string contents; // 具体内容

    QueuedPacket( const std::string & s_contents, uint64_t s_arrival_time )
        : arrival_time( s_arrival_time ), contents( s_contents )
    {}
};

#endif /* QUEUED_PACKET_HH */
