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
        cout << "6. Exit Request Module\n";

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
                cout << "\nReturning to Main Menu...\n";
                break;

            default:
                cout << "\nInvalid choice.\n";
        }

    } while(choice != 6);
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


    activeRequests.addRequest(
        nextID,
        village,
        people,
        1
    );


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


    activeRequests.addRequest(
        nextID,
        village,
        people,
        0
    );


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

    RequestNode *request;


    cout << "\n";
    cout << "====================================\n";
    cout << "         PROCESS NEXT REQUEST\n";
    cout << "====================================\n";


    /*
       Emergency requests are processed first.
    */

    if(!emergencyRequests.isEmpty())
    {
        id = emergencyRequests.remove();

        cout << "\nEmergency request selected.\n";
    }

    else if(!normalRequests.isEmpty())
    {
        id = normalRequests.dequeue();

        cout << "\nNormal request selected.\n";
    }

    else
    {
        cout << "\nNo requests available.\n";

        return;
    }


    request = activeRequests.findRequest(id);


    if(request != NULL)
    {
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

        history.push(id);


        /*
           Remove request from
           active Linked List.
        */

        activeRequests.deleteRequest(id);


        cout << "\nRequest processed successfully.\n";
    }
}


void RequestManagement::displayHistory()
{
    history.display();
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