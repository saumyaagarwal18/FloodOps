#ifndef REQUESTMANAGEMENT_H
#define REQUESTMANAGEMENT_H

#include <string>

#include "Queue.h"
#include "PriorityQueue.h"
#include "LinkedList.h"
#include "Stack.h"

void requestMenu();
using namespace std;


class RequestManagement
{
private:

    PriorityQueue emergencyRequests;

    Queue normalRequests;

    LinkedList activeRequests;

    Stack history;

    int nextID;


public:

    RequestManagement();


    void addEmergencyRequest();

    void addNormalRequest();


    void displayActiveRequests();

    void processNextRequest();


    void displayHistory();


    void displayEmergencyRequests();

    void displayNormalRequests();
};

#endif