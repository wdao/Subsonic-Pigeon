#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include "include/packet.h"
#include "include/packet_handle.h"
#include "include/server_storage.h"
#include "include/mySQLHandler.h"

using boost::asio::ip::tcp;

//----------------------------------------------------------------------
//General
typedef std::deque<packet> packet_queue;
//----------------------------------------------------------------------

//////////////////////////////////// Global Variables ////////////////////////////////////

server_storage GLOBAL;
packet_handle PKT_HANDLE;

//packet_queue read_msgs_queue;
bool TERMINATE = false;
bool DEBUG = false;


//////////////////////////////////// Global Variables ////////////////////////////////////

class client
{
public:
    //Needed for Share from this
    virtual ~client() {};
    virtual void deliver(const packet& msg) = 0;  //?

    char* clientname; //Human friendly name
    char* ident;
    int performance; //simple performace index
    //std::set<phone_ptr> phones_; //set of phones
};

//----------------------------------------------------------------------

typedef boost::shared_ptr<client> client_ptr;

//----------------------------------------------------------------------

//Incoming Queue
/*
MOVED TO GLOBAL.H
struct incoming_packet
{
    packet* msg;
    client_ptr* cli;
};

*/

//----------------------------------------------------------------------

class chat_room
{
public:
    void join(client_ptr participant)
    {
        participants_.insert(participant);
        std::for_each(recent_msgs_.begin(), recent_msgs_.end(), boost::bind(&client::deliver, participant, _1));
        std::cout << "Connection Established: '" << participant << "'\n";
    }

    void leave(client_ptr participant)
    {
        std::cout << "Client Disconnect: \n";
        participants_.erase(participant);
    }

    /*void deliver(const packet& msg)
    {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        std::for_each(participants_.begin(), participants_.end(), boost::bind(&client::deliver, _1, boost::ref(msg)));
    }

    void write(const packet msg, client_ptr cli)
    {
        std::cout << "Room Write Rquest\n";
        std::cout << "Request: '";
        std::cout.write(msg.body(), msg.body_length());
        std::cout << "'\n";
        std::cout << "To Client: '" << cli << "'\n";
        boost::bind(&client::deliver, cli, boost::ref(msg));
    }*/

private:
    std::set<client_ptr> participants_;
    enum { max_recent_msgs = 100 };
    packet_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session : public client, public boost::enable_shared_from_this<chat_session>
{
public:
    chat_session(boost::asio::io_service& io_service, chat_room& room) : socket_(io_service), room_(room)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        room_.join(shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), packet::header_length), boost::bind(&chat_session::handle_read_header, shared_from_this(), boost::asio::placeholders::error));

        //Send Registration Request
        std::cout << "Sending Welcome String\n";
        char* temp;
        temp = "'PACKET'"; //replace with peroper encoding
        packet msg;
        msg.body_length(std::strlen(temp));
        std::memcpy(msg.body(), temp, msg.body_length());
        msg.encode_header();
        deliver(msg);
    }

    void deliver(const packet& msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
        {
            /*std::cout << "Boost Deliver: ";
            std::cout.write(msg.body(), msg.body_length());
            std::cout <<"\n";
            std::cout << "On Socket: '" << &socket_ << "'\n";*/
            boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()), boost::bind(&chat_session::handle_write, shared_from_this(), boost::asio::placeholders::error));
        }
    }

    void handle_read_header(const boost::system::error_code& error)
    {
        if (!error && read_msg_.decode_header())
        {
            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.body_length()), boost::bind(&chat_session::handle_read_body, shared_from_this(), boost::asio::placeholders::error));
        }
        else
        {
            room_.leave(shared_from_this());
        }
    }

    void add_to_queue(packet inpkt, client_ptr cli)
    {
        incoming_packet incoming_msg;

        incoming_msg.msg = &inpkt;
        incoming_msg.cli = &cli;

        incoming_queue_msgs.push_back(incoming_msg);
    }

    void handle_read_body(const boost::system::error_code& error)
    {
        if (!error)
        {
            /*std::cout << "Boost Reading: ";
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";*/

            //incoming_packet incoming_msg;
            //incoming_msg.msg = &read_msg_;
            //incoming_msg.cli = shared_from_this();
            //incoming_queue_msgs.push_back(incoming_msg);

            add_to_queue(read_msg_, shared_from_this());

            boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), packet::header_length), boost::bind(&chat_session::handle_read_header, shared_from_this(), boost::asio::placeholders::error));
        }
        else
        {
            room_.leave(shared_from_this());
        }
    }

    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
                /*std::cout << "Boost Sending: ";
                std::cout.write(write_msgs_.front().body(), write_msgs_.front().body_length());
                std::cout <<"\n";
                std::cout << "On Socket: '" << &socket_ << "'\n";*/

                boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()), boost::bind(&chat_session::handle_write, shared_from_this(), boost::asio::placeholders::error));
            }
        }
        else
        {
            room_.leave(shared_from_this());
        }
    }



private:
    chat_room& room_;
    tcp::socket socket_;
    packet read_msg_;
    packet_queue write_msgs_;
};

//----------------------------------------------------------------------

//Server
typedef boost::shared_ptr<chat_session> chat_session_ptr;

class server
{
public:
    server(boost::asio::io_service& io_service, const tcp::endpoint& endpoint) : io_service_(io_service), acceptor_(io_service, endpoint)
    {
        start_accept();
    }

    void start_accept()
    {
        chat_session_ptr new_session(new chat_session(io_service_, room_));
        acceptor_.async_accept(new_session->socket(), boost::bind(&server::handle_accept, this, new_session, boost::asio::placeholders::error));
    }

    void handle_accept(chat_session_ptr session, const boost::system::error_code& error)
    {
        if (!error)
        {
            session->start();
        }

        start_accept();
    }

    void write(const packet& msg, client_ptr cli)
    {
        /*std::cout << "Server Write Rquest\n";
        std::cout << "Request: '";
        std::cout.write(msg.body(), msg.body_length());
        std::cout << "'\n";
        std::cout << "To Client: '" << cli << "'\n";*/
        //room_.write(msg, cli);
        io_service_.post(boost::bind(&client::deliver, cli, boost::ref(msg)));
    }

    void close()
    {

    }

private:
    boost::asio::io_service& io_service_;
    tcp::acceptor acceptor_;
    chat_room room_;
};

//////////////////////////////////// MAIN PROGRAM ////////////////////////////////////////////////////////

typedef boost::shared_ptr<server> server_ptr;
server_ptr global_server;

//Generic Packet
void send_packet(client_ptr cli, packet outpkt)
{
    global_server->write(outpkt, cli);
}

//Incoming Packet Handler

void incoming_watcher()
{
    packet* inpkt;
    packet outpkt;
    client_ptr* cli;

    std::cout << "Incoming Packet Handler Started\n";

    while (!TERMINATE)
    {
        if (!(incoming_queue_msgs.empty()))
        {
            /*std::cout << incoming_queue_msgs.size();
            std::cout << " Packet(s) in the queue. Packet Pulled From queue\n";*/

            inpkt = incoming_queue_msgs.front().msg;
            cli = incoming_queue_msgs.front().cli;

            /*std::cout << "Recieved message: '";
            std::cout.write(inpkt->body(), inpkt->body_length());
            std::cout << "From Client: '" << *cli << "'\n";*/

            outpkt = PKT_HANDLE.incoming_packet(inpkt);

            if (outpkt.body() != "")
            {
                /*std::cout << "Sending Reply: '";
                std::cout.write(outpkt.body(), outpkt.body_length());
                std::cout << "'\n";*/

                send_packet(*cli, outpkt);

            }else
            {
                std::cout << "Not Sending Reply\n";
            }

            incoming_queue_msgs.pop_front();
        }
    }

    std::cout << "Incoming Packet TERMINATED\n";
}

//Mysql

//Command Interfcae

//INT MAIN
int main(int argc, char* argv[])
{
    PKT_HANDLE.GLOBAL = &GLOBAL;
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: Subsonic Pigeon SMS Server <port>\n";
            return 1;
        }

        boost::asio::io_service io_service;

        using namespace std; // For atoi.
        tcp::endpoint endpoint(tcp::v4(), atoi(argv[1]));
        server_ptr server1(new server(io_service, endpoint));
        global_server = server1;

        //Start Async Service
        boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));
        std::cout << "Subsonic Pigeon SMS Server\n--------------------------\nServer Start\nPort "  << atoi(argv[1]) << " Open\n";

        //Start AUX services
        boost::thread incoming_T(incoming_watcher);
        //boost::thread mysql_T(mysql_handler);

		
		
		//start SQL handler service
		boost::thread SQLHandler(outboxHandler());
        std::cout << "Command Interface Open - Type help\n";

        char line[packet::max_body_length + 1];
        //while (std::cin.getline(line, packet::max_body_length + 1))
        while(1)
        {
            std::cout << "-\n";
            sleep(5);
        }

        server1->close();
		//these are all attached threads
        t.join();

    }

    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
