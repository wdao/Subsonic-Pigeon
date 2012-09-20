#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <string.h> //in order to 
#include <exception>
#include <cstdlib>
#include <sys/wait.h>
#include <queue>
#include "global.h"
#include "include/packet.h"
#include "include/packet_handle.h"
#include "include/server_storage.h"

using namespace soci;
using namespace std;


/*class messageinfo
{

    public:
    string message, cellnum;
    void setValues (string, string);
    ~messageinfo();
};

void messageinfo::setValues(string mess, string num)
{
    message = mess;
    cellnum = num;
}

messageinfo::~messageinfo()
{
    //delete this;
}

//queue <messageinfo *> myQueue;

*/

void AddQueue (string message, string cellnum)
{
    //messageinfo *add;
    //add = new messageinfo;
    //add->setValues(message, cellnum);
    //myQueue.push(add);

    //messageinfo *repeat;
    //repeat = myQueue.front();
    //cout <<repeat->message <<" " <<repeat->cellnum<<endl;
    //myQueue.pop();

    //delete repeat;
	
	string cat = string message + "SOMETHING HERE" + cellnum;
	
	char *c_cat = c_str(add); //convert add to c-string char array
	
	struct incoming_packet *add;
	//add = new incoming_packet;
	packet *msg = new packet;
	msg->data_ = c_cat;
	msg->body_length_ = sizeof(c_cat);
	
	if (msg->body_length_ > 515) msg->done = 1;
	else msg->done = 0;
	
	//add->msg = msg;
	//add->cli	//don't really know what this is
	
	//int size = sizeof(add);
	
	//incoming_queue_msgs.push_back(add);
	
	client_ptr cli //not sure what this does
	send_packet(cli, msg);
	

}


void outboxHandler()
{
    session sql(mysql, "db=coop user=pigeon password='pigeonscantspeak' host=192.168.1.102 port=3306");

    int count;
    sql <<"select count(*) from outbox", into(count);

/*
    string text;
    string text2 = "Text Decoded";
    string TextDecoded;
    int id;
    sql <<"select TextDecoded from outbox", into(TextDecoded);
    sql <<"select id from outbox", into(id);
    cout <<TextDecoded<<endl;

    cout<<id<<endl;
*/

    while (1)
    {
        rowset<row> rs = (sql.prepare << "select idmessage, idaliases from outbox");
        cout <<"HELLO"<<endl;
        for (rowset<row>::const_iterator it = rs.begin(); it != rs.end(); ++it)
        {
            row const& rows = *it;

            if (rows.get<int>(0) == 0) continue;
            cout << "IDMESSAGE: " << rows.get<int>(0) << endl;
            cout << "IDALIASES: " << rows.get<int>(1) << endl;

            int idmessage = rows.get<int>(0);
            int idaliases = rows.get<int>(1);
            string cellnum;

            string message;
            sql <<"select message from message where idmessage ="<<idmessage, into (message);
            cout <<message<<endl;


            sql <<"SELECT celltext.number FROM celltext INNER JOIN aliases ON celltext.idusers = aliases.idusers INNER JOIN outbox ON aliases.idaliaes = outbox.idaliases WHERE outbox.idaliases="<<idaliases, into(cellnum);
            if (cellnum == ""){
                cout <<"No Phone Number attached"<<endl;
                continue;
            }else{
                cout <<cellnum<<endl;
            }

            AddQueue(message, cellnum);

            //to delete the message from the sql server.
            //sql <<"Delete from outbox where idmessage = "<< idmessage << "and idaliases = "<<idaliases;



        }
    }
        cout <<"Exiting Thread"<<endl;

}

/*int main()
{

    int pID;
    pID = fork();
    if (pID == 0){
        outboxHandler();
    }
    int x;
    waitpid(pID, &x, 0);
}
*/
