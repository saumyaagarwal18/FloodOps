#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <string>

using namespace std;


struct RequestNode
{
    int id;

    string village;

    int people;

    int priority;

    RequestNode *next;
};


class LinkedList
{
private:

    RequestNode *head;

public:

    LinkedList();

    void addRequest(
        int id,
        string village,
        int people,
        int priority
    );

    RequestNode* findRequest(int id);

    void deleteRequest(int id);

    void display();
};

#endif