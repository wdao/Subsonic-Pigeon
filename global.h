#include <queue>
#include "include/packet.h"

#ifndef GLOBAL_H_INCLUDED
#define GLOBAL_H_INCLUDED

std::queue<packet> inbox;
std::queue<packet> outbox;


struct incoming_packet
{
    packet* msg;
    client_ptr* cli; //WTF is this???????????????????????????
};


typedef std::deque<incoming_packet> incoming_queue;
incoming_queue incoming_queue_msgs;


#endif // GLOBAL_H_INCLUDED
