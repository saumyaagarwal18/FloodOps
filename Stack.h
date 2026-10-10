#ifndef STACK_H
#define STACK_H

class Stack
{
private:

    int data[50];

    int top;

public:

    Stack();

    void push(int x);

    int pop();

    bool isEmpty();

    void display();
};

#endif