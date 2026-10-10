#include <iostream>
#include "LinkedList.h"

using namespace std;


LinkedList::LinkedList()
{
    head = NULL;
}


void LinkedList::addRequest(
    int id,
    string village,
    int people,
    int priority
)
{
    RequestNode *p;

    p = new RequestNode;

    p->id = id;

    p->village = village;

    p->people = people;

    p->priority = priority;

    p->next = NULL;


    if(head == NULL)
    {
        head = p;
    }
    else
    {
        RequestNode *temp = head;

        while(temp->next != NULL)
        {
            temp = temp->next;
        }

        temp->next = p;
    }
}


RequestNode* LinkedList::findRequest(int id)
{
    RequestNode *temp = head;

    while(temp != NULL)
    {
        if(temp->id == id)
        {
            return temp;
        }

        temp = temp->next;
    }

    return NULL;
}


void LinkedList::deleteRequest(int id)
{
    if(head == NULL)
    {
        return;
    }


    if(head->id == id)
    {
        RequestNode *temp = head;

        head = head->next;

        delete temp;

        return;
    }


    RequestNode *prev = head;

    RequestNode *temp = head->next;


    while(temp != NULL)
    {
        if(temp->id == id)
        {
            prev->next = temp->next;

            delete temp;

            return;
        }

        prev = temp;

        temp = temp->next;
    }
}


void LinkedList::display()
{
    if(head == NULL)
    {
        cout << "\nNo active requests.\n";
        return;
    }


    RequestNode *temp = head;


    cout << "\n";
    cout << "====================================\n";
    cout << "       ACTIVE RELIEF REQUESTS\n";
    cout << "====================================\n";


    while(temp != NULL)
    {
        cout << "Request ID      : "
             << temp->id << endl;

        cout << "Village         : "
             << temp->village << endl;

        cout << "People Affected : "
             << temp->people << endl;


        if(temp->priority == 1)
        {
            cout << "Priority        : EMERGENCY\n";
        }
        else
        {
            cout << "Priority        : NORMAL\n";
        }

        cout << "------------------------------------\n";


        temp = temp->next;
    }
}