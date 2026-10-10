#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

class PriorityQueue
{
private:

    int data[50];

    int priority[50];

    int size;

public:

    PriorityQueue();

    void insert(int id, int p);

    int remove();

    bool isEmpty();

    void display();
};

#endif