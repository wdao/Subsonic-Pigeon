
#ifndef MYSQLHANDLER
#define MYSQLHANDLER

#include <queue>

class messageinfo
{

    public:
    string message, cellnum;
    void setValues (string, string);
    ~messageinfo();
};


void messageinfo::setValues(string mess, string num);

messageinfo::~messageinfo();


void AddQueue (string message, string cellnum);


void outboxHandler()

//data structures

queue <messageinfo *> myQueue;

#endif MYSQLHANDLER
