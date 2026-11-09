#include <iostream>

#include "RequestManagement.h"

using namespace std;

void requestMenu()
{
    RequestManagement manager;
    int choice;

    do
    {
        cout << "\n";
        cout << "====================================\n";
        cout << "       REQUEST MANAGEMENT\n";
        cout << "====================================\n";

        cout << "\n";
        cout << "1. Add Emergency Request\n";
        cout << "2. Add Normal Request\n";
        cout << "3. Process Next Request\n";
        cout << "4. Display Emergency Requests\n";
        cout << "5. Display Normal Requests\n";
        cout << "6. Display Processed History\n";
        cout << "7. Add Shelter\n";
        cout << "8. Display Shelter Capacities\n";
        cout << "9. Release Evacuees\n";
        cout << "10. Exit Request Module\n";

        cout << "\nEnter choice: ";
        cin >> choice;

        switch(choice)
        {
            case 1:
                manager.addEmergencyRequest();
                break;

            case 2:
                manager.addNormalRequest();
                break;

            case 3:
                manager.processNextRequest();
                break;

            case 4:
                manager.displayEmergencyRequests();
                break;

            case 5:
                manager.displayNormalRequests();
                break;

            case 6:
                manager.displayHistory();
                break;

            case 7:
                manager.addShelter();
                break;

            case 8:
                manager.displayShelters();
                break;

            case 9:
            {
                int requestId;
                int evacuees;
                cout << "\nProcessed request ID: ";
                cin >> requestId;
                cout << "Number of evacuees to release: ";
                cin >> evacuees;
                if (manager.releaseEvacuees(requestId, evacuees))
                {
                    cout << "\nShelter capacity released.\n";
                }
                else
                {
                    cout << "\nNo matching allocation for that release amount.\n";
                }
                break;
            }

            case 10:
                cout << "\nReturning to Main Menu...\n";
                break;

            default:
                cout << "\nInvalid choice.\n";
        }

    } while(choice != 10);
}


RequestManagement::RequestManagement()
{
    nextID = 1;
}


void RequestManagement::addEmergencyRequest()
{
    string village;

    int people;


    cout << "\n";
    cout << "====================================\n";
    cout << "      ADD EMERGENCY REQUEST\n";
    cout << "====================================\n";


    cout << "Enter village name: ";
    cin >> village;


    cout << "Enter number of people affected: ";
    cin >> people;


    if (people <= 0 ||
        !requestsById.insert(ReliefRequest{nextID, village, people, 1}))
    {
        cout << "\nEnter a positive affected-person count.\n";
        return;
    }

    activeRequests.addRequest(nextID, village, people, 1);


    /*
       1 = Emergency priority
    */

    emergencyRequests.insert(
        nextID,
        1
    );


    cout << "\nEmergency request added.\n";

    cout << "Request ID: "
         << nextID
         << endl;


    nextID++;
}


void RequestManagement::addNormalRequest()
{
    string village;

    int people;


    cout << "\n";
    cout << "====================================\n";
    cout << "        ADD NORMAL REQUEST\n";
    cout << "====================================\n";


    cout << "Enter village name: ";
    cin >> village;


    cout << "Enter number of people affected: ";
    cin >> people;


    if (people <= 0 ||
        !requestsById.insert(ReliefRequest{nextID, village, people, 0}))
    {
        cout << "\nEnter a positive affected-person count.\n";
        return;
    }

    activeRequests.addRequest(nextID, village, people, 0);


    normalRequests.enqueue(nextID);


    cout << "\nNormal request added.\n";

    cout << "Request ID: "
         << nextID
         << endl;


    nextID++;
}


void RequestManagement::displayActiveRequests()
{
    activeRequests.display();
}


void RequestManagement::processNextRequest()
{
    int id;
    const bool emergency = !emergencyRequests.isEmpty();


    cout << "\n";
    cout << "====================================\n";
    cout << "         PROCESS NEXT REQUEST\n";
    cout << "====================================\n";


    /*
       Emergency requests are processed first.
    */

    if(emergency)
    {
        id = emergencyRequests.peek();

        cout << "\nEmergency request selected.\n";
    }

    else if(!normalRequests.isEmpty())
    {
        id = normalRequests.peek();

        cout << "\nNormal request selected.\n";
    }

    else
    {
        cout << "\nNo requests available.\n";

        return;
    }


    const ReliefRequest *request = requestsById.find(id);

    if(request != NULL)
    {
        vector<ShelterAssignment> assignments;
        if (!shelterTracker.allocate(id, request->village, request->people, assignments))
        {
            cout << "\nInsufficient shelter capacity. The request remains queued.\n";
            return;
        }

        if(emergency)
        {
            emergencyRequests.remove();
        }
        else
        {
            normalRequests.dequeue();
        }

        cout << "\n";
        cout << "Request ID      : "
             << request->id << endl;

        cout << "Village         : "
             << request->village << endl;

        cout << "People Affected : "
             << request->people << endl;


        if(request->priority == 1)
        {
            cout << "Priority        : EMERGENCY\n";
        }
        else
        {
            cout << "Priority        : NORMAL\n";
        }


        cout << "Status          : PROCESSED\n";


        /*
           Add request to history.
        */

        OperationRecord operation;
        operation.requestId = id;
        operation.village = request->village;
        operation.people = request->people;
        operation.priority = request->priority;
        operation.assignments = assignments;
        history.push(operation);

        cout << "Shelter Assignments:\n";
        for (size_t i = 0; i < assignments.size(); ++i)
        {
            cout << "  " << assignments[i].shelterName << ": "
                 << assignments[i].evacuees << " evacuees\n";
        }


        /*
           Remove request from
           active Linked List.
        */

        activeRequests.deleteRequest(id);
        requestsById.erase(id);


        cout << "\nRequest processed successfully.\n";
    }
}


void RequestManagement::displayHistory()
{
    history.display();
}

void RequestManagement::addShelter()
{
    string name;
    int capacity;

    cout << "\nShelter name: ";
    cin >> name;
    cout << "Shelter capacity: ";
    cin >> capacity;

    if (registerShelter(name, capacity))
    {
        cout << "\nShelter registered.\n";
    }
    else
    {
        cout << "\nShelter name must be unique and capacity must be positive.\n";
    }
}

bool RequestManagement::registerShelter(const string &name, int capacity)
{
    return shelterTracker.addShelter(name, capacity);
}

bool RequestManagement::releaseEvacuees(int requestId, int evacuees)
{
    return shelterTracker.release(requestId, evacuees);
}

void RequestManagement::displayShelters()
{
    const vector<ShelterCapacity> shelters = shelterTracker.getShelters();
    if (shelters.empty())
    {
        cout << "\nNo shelters registered.\n";
        return;
    }

    cout << "\nShelter Capacity:\n";
    for (size_t i = 0; i < shelters.size(); ++i)
    {
        cout << shelters[i].name << ": " << shelters[i].occupied
             << "/" << shelters[i].capacity << " occupied\n";
    }
}


void RequestManagement::displayEmergencyRequests()
{
    cout << "\n--- EMERGENCY REQUESTS ---\n";

    emergencyRequests.display();
}


void RequestManagement::displayNormalRequests()
{
    cout << "\n--- NORMAL REQUESTS ---\n";

    normalRequests.display();
}