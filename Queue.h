#ifndef QUEUE_H
#define QUEUE_H

class Queue
{
private:

    int data[50];

    int front;
    int rear;

public:

    Queue();

    void enqueue(int x);

    int dequeue();

    bool isEmpty();

    void display();
};

#endif