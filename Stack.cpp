#include <iostream>
#include "Stack.h"

using namespace std;

Stack::Stack()
{
    topNode = NULL;
}

Stack::~Stack()
{
    while (topNode != NULL)
    {
        Node *next = topNode->next;
        delete topNode;
        topNode = next;
    }
}

void Stack::push(const OperationRecord &operation)
{
    Node *node = new Node(operation);
    node->next = topNode;
    topNode = node;
}

void Stack::push(int requestId)
{
    OperationRecord operation;
    operation.requestId = requestId;
    operation.people = 0;
    operation.priority = 0;
    push(operation);
}

bool Stack::pop(OperationRecord &operation)
{
    if (isEmpty())
    {
        return false;
    }

    Node *node = topNode;
    operation = node->operation;
    topNode = node->next;
    delete node;
    return true;
}

bool Stack::top(OperationRecord &operation) const
{
    if (isEmpty())
    {
        return false;
    }
    operation = topNode->operation;
    return true;
}

bool Stack::isEmpty() const
{
    return topNode == NULL;
}

std::vector<OperationRecord> Stack::getAll() const
{
    std::vector<OperationRecord> operations;
    for (const Node *node = topNode; node != NULL; node = node->next)
    {
        operations.push_back(node->operation);
    }
    return operations;
}

void Stack::display()
{
    if (isEmpty())
    {
        cout << "History is empty.\n";
        return;
    }

    cout << "\nProcessed Relief Operation History (newest first):\n";
    for (const Node *node = topNode; node != NULL; node = node->next)
    {
        cout << "Request ID: " << node->operation.requestId;
        if (!node->operation.village.empty())
        {
            cout << "  Village: " << node->operation.village
                 << "  Evacuees: " << node->operation.people;
            for (std::size_t i = 0; i < node->operation.assignments.size(); ++i)
            {
                cout << "\n  " << node->operation.assignments[i].shelterName
                     << ": " << node->operation.assignments[i].evacuees;
            }
        }
        cout << endl;
    }
}