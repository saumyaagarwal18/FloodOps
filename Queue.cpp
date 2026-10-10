#include <iostream>
#include "Queue.h"

using namespace std;


Queue::Queue()
{
    front = -1;
    rear = -1;
}


void Queue::enqueue(int x)
{
    if(rear == 49)
    {
        cout << "Queue is full.\n";
        return;
    }

    if(front == -1)
    {
        front = 0;
    }

    rear++;

    data[rear] = x;
}


int Queue::dequeue()
{
    if(isEmpty())
    {
        return -1;
    }

    int x = data[front];

    front++;


    if(front > rear)
    {
        front = -1;
        rear = -1;
    }

    return x;
}


bool Queue::isEmpty()
{
    if(front == -1)
    {
        return true;
    }

    return false;
}


void Queue::display()
{
    if(isEmpty())
    {
        cout << "Queue is empty.\n";
        return;
    }

    for(int i = front; i <= rear; i++)
    {
        cout << data[i] << " ";
    }

    cout << endl;
}