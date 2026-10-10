#include <iostream>
#include "PriorityQueue.h"

using namespace std;


PriorityQueue::PriorityQueue()
{
    size = 0;
}


void PriorityQueue::insert(int id, int p)
{
    if(size == 50)
    {
        cout << "Priority Queue is full.\n";
        return;
    }

    int i;

    /*
       Move lower-priority requests
       one position ahead.
    */

    i = size - 1;

    while(i >= 0 && priority[i] < p)
    {
        data[i + 1] = data[i];

        priority[i + 1] = priority[i];

        i--;
    }

    data[i + 1] = id;

    priority[i + 1] = p;

    size++;
}


int PriorityQueue::remove()
{
    if(isEmpty())
    {
        return -1;
    }

    int id = data[0];

    for(int i = 0; i < size - 1; i++)
    {
        data[i] = data[i + 1];

        priority[i] = priority[i + 1];
    }

    size--;

    return id;
}


bool PriorityQueue::isEmpty()
{
    if(size == 0)
    {
        return true;
    }

    return false;
}


void PriorityQueue::display()
{
    if(isEmpty())
    {
        cout << "Priority Queue is empty.\n";
        return;
    }

    for(int i = 0; i < size; i++)
    {
        cout << "Request ID: "
             << data[i]
             << "  Priority: "
             << priority[i]
             << endl;
    }
}