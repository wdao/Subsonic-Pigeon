#ifndef PACKET_HANDLE_H_INCLUDED
#define PACKET_HANDLE_H_INCLUDED

#include "packet.h"
#include "server_storage.h"
#include <iostream>
#include <cstdlib>
#include <string>

class packet_handle
{
    public:

    server_storage* GLOBAL;

    //Incoming Packets Start Here
    packet incoming_packet(packet* inpkt)
    {
        char * in_msg;

        in_msg = inpkt->body();

        //Packet Switcher
        switch (in_msg[0]) //packet type identifer
        {
            case packet::join_server: return join_server(in_msg);
                break;
            case packet::sms_message: return sms_message(in_msg);
                break;
            case packet::reg_phone: return reg_phone(in_msg);
                break;
            case packet::partner_leaving: return partner_leaving(in_msg);
                break;
            case packet::stats: return stats(in_msg);
                break;
            case packet::heartbeat: return heartbeat(in_msg);
                break;
            default: return create_packet(in_msg, packet::echo);
                break;
        }
    }

    //Packet Creator - will be called by the main program and functions frim within this class
    packet create_packet(char* out_msg, char msg_type)
    {
        packet outpkt;
        char* temp;

        temp = strcat(msg_type, out_msg);
        packet msg;
        msg.body_length(std::strlen(temp));
        std::memcpy(msg.body(), temp, msg.body_length());
        msg.encode_header();
        return outpkt;
    }

    private:



    //Type Handlers

    packet join_server(char* in_msg)
    {
        char* new_msg;
        new_msg = in_msg+1;

        switch(new_msg[0])
        {
            case 'W':: return
        }

    }

    packet sms_message(char* in_msg)
    {

    }

    packet reg_phone(char* in_msg)
    {

    }

    packet partner_leaving(char* in_msg)
    {

    }

    packet stats(char* in_msg)
    {

    }


};


#endif // PACKET_HANDLE_H_INCLUDED
