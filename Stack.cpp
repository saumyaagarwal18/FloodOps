#include <iostream>
#include "Stack.h"

using namespace std;


Stack::Stack()
{
    top = -1;
}


void Stack::push(int x)
{
    if(top == 49)
    {
        cout << "Stack is full.\n";
        return;
    }

    top++;

    data[top] = x;
}


int Stack::pop()
{
    if(isEmpty())
    {
        return -1;
    }

    int x = data[top];

    top--;

    return x;
}


bool Stack::isEmpty()
{
    if(top == -1)
    {
        return true;
    }

    return false;
}


void Stack::display()
{
    if(isEmpty())
    {
        cout << "History is empty.\n";
        return;
    }


    cout << "\nProcessed Request History:\n";


    for(int i = top; i >= 0; i--)
    {
        cout << "Request ID: "
             << data[i]
             << endl;
    }
}