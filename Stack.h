#ifndef STACK_H
#define STACK_H

#include <string>
#include <vector>

#include "ShelterCapacityTracker.h"

struct OperationRecord
{
    int requestId;
    std::string village;
    int people;
    int priority;
    std::vector<ShelterAssignment> assignments;
};

class Stack
{
private:
    struct Node
    {
        OperationRecord operation;
        Node *next;

        explicit Node(const OperationRecord &value) : operation(value), next(NULL) {}
    };

    Node *topNode;

public:
    Stack();
    ~Stack();

    Stack(const Stack &) = delete;
    Stack &operator=(const Stack &) = delete;

    void push(const OperationRecord &operation);
    void push(int requestId);
    bool pop(OperationRecord &operation);
    bool top(OperationRecord &operation) const;

    bool isEmpty() const;
    std::vector<OperationRecord> getAll() const;

    void display();
};

#endif