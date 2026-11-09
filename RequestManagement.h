#ifndef REQUESTMANAGEMENT_H
#define REQUESTMANAGEMENT_H

#include <string>

#include "Queue.h"
#include "PriorityQueue.h"
#include "LinkedList.h"
#include "Stack.h"
#include "RequestHashTable.h"
#include "ShelterCapacityTracker.h"

void requestMenu();
using namespace std;


class RequestManagement
{
private:

    PriorityQueue emergencyRequests;

    Queue normalRequests;

    LinkedList activeRequests;

    Stack history;

    RequestHashTable requestsById;

    ShelterCapacityTracker shelterTracker;

    int nextID;


public:

    RequestManagement();


    void addEmergencyRequest();

    void addNormalRequest();


    void displayActiveRequests();

    void processNextRequest();


    void displayHistory();

    void addShelter();

    void displayShelters();

    bool registerShelter(const string &name, int capacity);

    bool releaseEvacuees(int requestId, int evacuees);


    void displayEmergencyRequests();

    void displayNormalRequests();
};

#endif