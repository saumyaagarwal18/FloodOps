#include "LocationManagement.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <limits>
#include <vector>

using namespace std;


// ==========================================
// LOCATION TREE
// ==========================================

LocationNode *districtRoot = NULL;


// ==========================================
// ROAD NETWORK
// ==========================================

const int MAX_LOCATIONS = 50;

// Stores names of locations used in roads
string locationNames[MAX_LOCATIONS];

int locationCount = 0;

/*
    Road values:

    0 = No road
    1 = Open road
    2 = Blocked road
*/

int road[MAX_LOCATIONS][MAX_LOCATIONS] = {0};


// ==========================================
// CLEAR INPUT BUFFER
// ==========================================

void clearInput()
{
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}


// ==========================================
// ADD CHILD NODE
// ==========================================

void addChild(LocationNode *parent, LocationNode *newNode)
{
    if(parent->child == NULL)
    {
        parent->child = newNode;
    }
    else
    {
        LocationNode *temp = parent->child;

        while(temp->sibling != NULL)
        {
            temp = temp->sibling;
        }

        temp->sibling = newNode;
    }
}


// ==========================================
// FIND NODE IN LOCATION TREE
// ==========================================

LocationNode* findNode(LocationNode *root, string name)
{
    if(root == NULL)
    {
        return NULL;
    }

    if(root->name == name)
    {
        return root;
    }

    LocationNode *result;

    result = findNode(root->child, name);

    if(result != NULL)
    {
        return result;
    }

    return findNode(root->sibling, name);
}


// ==========================================
// ENTER LOCATION
// ==========================================

void enterLocation()
{
    string districtName;
    string villageName;
    string areaName;

    cout << "\n========================================\n";
    cout << "            ENTER LOCATION\n";
    cout << "========================================\n";

    clearInput();

    cout << "\nEnter district name: ";
    getline(cin, districtName);

    cout << "Enter village name: ";
    getline(cin, villageName);

    cout << "Enter area name: ";
    getline(cin, areaName);


    // --------------------------------------
    // CHECK EMPTY INPUT
    // --------------------------------------

    if(districtName.empty() ||
       villageName.empty() ||
       areaName.empty())
    {
        cout << "\nLocation information cannot be empty.\n";
        return;
    }


    // --------------------------------------
    // CREATE DISTRICT
    // --------------------------------------

    if(districtRoot == NULL)
    {
        districtRoot =
            new LocationNode(districtName, "District");

        LocationNode *village =
            new LocationNode(villageName, "Village");

        LocationNode *area =
            new LocationNode(areaName, "Area");

        addChild(districtRoot, village);

        addChild(village, area);

        cout << "\nLocation added successfully.\n";

        return;
    }


    // --------------------------------------
    // FIND DISTRICT
    // --------------------------------------

    LocationNode *district =
        findNode(districtRoot, districtName);


    // --------------------------------------
    // DISTRICT DOES NOT EXIST
    // --------------------------------------

    if(district == NULL)
    {
        cout << "\nDistrict not found in the existing hierarchy.\n";

        cout << "Adding a new district...\n";

        LocationNode *newDistrict =
            new LocationNode(districtName, "District");

        LocationNode *village =
            new LocationNode(villageName, "Village");

        LocationNode *area =
            new LocationNode(areaName, "Area");

        addChild(districtRoot, newDistrict);

        addChild(newDistrict, village);

        addChild(village, area);

        cout << "\nLocation added successfully.\n";

        return;
    }


    // --------------------------------------
    // FIND VILLAGE UNDER DISTRICT
    // --------------------------------------

    LocationNode *village = district->child;

    while(village != NULL)
    {
        if(village->name == villageName)
        {
            break;
        }

        village = village->sibling;
    }


    // --------------------------------------
    // VILLAGE DOES NOT EXIST
    // --------------------------------------

    if(village == NULL)
    {
        village =
            new LocationNode(villageName, "Village");

        addChild(district, village);

        LocationNode *area =
            new LocationNode(areaName, "Area");

        addChild(village, area);

        cout << "\nLocation added successfully.\n";

        return;
    }


    // --------------------------------------
    // CHECK AREA
    // --------------------------------------

    LocationNode *area = village->child;

    while(area != NULL)
    {
        if(area->name == areaName)
        {
            cout << "\nArea already exists under this village.\n";
            return;
        }

        area = area->sibling;
    }


    // --------------------------------------
    // ADD NEW AREA
    // --------------------------------------

    LocationNode *newArea =
        new LocationNode(areaName, "Area");

    addChild(village, newArea);

    cout << "\nLocation added successfully.\n";
}


// ==========================================
// PRINT LOCATION TREE
// ==========================================

void printTree(LocationNode *root, int level)
{
    if(root == NULL)
    {
        return;
    }

    for(int i = 0; i < level; i++)
    {
        cout << "    ";
    }

    if(level == 0)
    {
        cout << root->name
             << " [" << root->type << "]"
             << endl;
    }
    else
    {
        cout << "+-- "
             << root->name
             << " [" << root->type << "]"
             << endl;
    }

    printTree(root->child, level + 1);

    printTree(root->sibling, level);
}


// ==========================================
// DISPLAY LOCATION HIERARCHY
// ==========================================

void displayHierarchy()
{
    cout << "\n========================================\n";
    cout << "          LOCATION HIERARCHY\n";
    cout << "========================================\n\n";

    if(districtRoot == NULL)
    {
        cout << "No locations added yet.\n";
        return;
    }

    printTree(districtRoot, 0);
}


// ==========================================
// GET LOCATION ID
// ==========================================

int getLocationID(string name)
{
    for(int i = 0; i < locationCount; i++)
    {
        if(locationNames[i] == name)
        {
            return i;
        }
    }

    if(locationCount >= MAX_LOCATIONS)
    {
        return -1;
    }

    locationNames[locationCount] = name;

    locationCount++;

    return locationCount - 1;
}


// ==========================================
// FIND LOCATION ID
// ==========================================

int findLocation(string name)
{
    for(int i = 0; i < locationCount; i++)
    {
        if(locationNames[i] == name)
        {
            return i;
        }
    }

    return -1;
}


// ==========================================
// ADD ROAD
// ==========================================

void addRoad()
{
    string source;
    string destination;

    cout << "\n========================================\n";
    cout << "              ADD ROAD\n";
    cout << "========================================\n";

    clearInput();

    cout << "\nEnter source location: ";
    getline(cin, source);

    cout << "Enter destination location: ";
    getline(cin, destination);


    if(source.empty() || destination.empty())
    {
        cout << "\nLocation name cannot be empty.\n";
        return;
    }


    if(source == destination)
    {
        cout << "\nSource and destination cannot be the same.\n";
        return;
    }


    // Check source in location hierarchy
    if(districtRoot == NULL ||
       findNode(districtRoot, source) == NULL)
    {
        cout << "\nSource location does not exist.\n";
        return;
    }


    // Check destination in location hierarchy
    if(findNode(districtRoot, destination) == NULL)
    {
        cout << "\nDestination location does not exist.\n";
        return;
    }


    int s = getLocationID(source);

    int d = getLocationID(destination);


    if(s == -1 || d == -1)
    {
        cout << "\nMaximum number of locations reached.\n";
        return;
    }


    if(road[s][d] == 1)
    {
        cout << "\nRoad already exists and is open.\n";
        return;
    }


    if(road[s][d] == 2)
    {
        cout << "\nRoad is currently blocked.\n";
        cout << "Use Open Road to open it.\n";
        return;
    }


    // Undirected graph
    road[s][d] = 1;
    road[d][s] = 1;


    cout << "\nRoad added successfully.\n";
}


// ==========================================
// BLOCK ROAD
// ==========================================

void blockRoad()
{
    string source;
    string destination;

    cout << "\n========================================\n";
    cout << "             BLOCK ROAD\n";
    cout << "========================================\n";

    clearInput();

    cout << "\nEnter source location: ";
    getline(cin, source);

    cout << "Enter destination location: ";
    getline(cin, destination);


    int s = findLocation(source);

    int d = findLocation(destination);


    if(s == -1 || d == -1)
    {
        cout << "\nRoad not found.\n";
        return;
    }


    if(road[s][d] == 0)
    {
        cout << "\nRoad does not exist.\n";
        return;
    }


    if(road[s][d] == 2)
    {
        cout << "\nRoad is already blocked.\n";
        return;
    }


    road[s][d] = 2;
    road[d][s] = 2;


    cout << "\nRoad blocked successfully.\n";
}


// ==========================================
// OPEN ROAD
// ==========================================

void openRoad()
{
    string source;
    string destination;

    cout << "\n========================================\n";
    cout << "              OPEN ROAD\n";
    cout << "========================================\n";

    clearInput();

    cout << "\nEnter source location: ";
    getline(cin, source);

    cout << "Enter destination location: ";
    getline(cin, destination);


    int s = findLocation(source);

    int d = findLocation(destination);


    if(s == -1 || d == -1)
    {
        cout << "\nRoad not found.\n";
        return;
    }


    if(road[s][d] == 0)
    {
        cout << "\nRoad does not exist.\n";
        return;
    }


    if(road[s][d] == 1)
    {
        cout << "\nRoad is already open.\n";
        return;
    }


    road[s][d] = 1;
    road[d][s] = 1;


    cout << "\nRoad opened successfully.\n";
}


// ==========================================
// DISPLAY ROAD NETWORK
// ==========================================

void displayRoadNetwork()
{
    cout << "\n========================================\n";
    cout << "             ROAD NETWORK\n";
    cout << "========================================\n";


    if(locationCount == 0)
    {
        cout << "\nNo roads added yet.\n";
        return;
    }


    bool found = false;


    for(int i = 0; i < locationCount; i++)
    {
        for(int j = i + 1; j < locationCount; j++)
        {
            if(road[i][j] != 0)
            {
                found = true;


                cout << "\n"
                     << locationNames[i]
                     << " <--> "
                     << locationNames[j];


                if(road[i][j] == 1)
                {
                    cout << "   [OPEN]";
                }
                else if(road[i][j] == 2)
                {
                    cout << "   [BLOCKED]";
                }
            }
        }
    }


    if(!found)
    {
        cout << "\nNo roads added yet.\n";
    }


    cout << endl;
}


// ==========================================
// BFS REACHABILITY
// ==========================================

bool isReachable(int source, int destination)
{
    return !findRoute(source, destination).empty();
}


std::vector<int> findRoute(int source, int destination)
{
    if(source < 0 || destination < 0 ||
       source >= locationCount || destination >= locationCount ||
       source >= MAX_LOCATIONS || destination >= MAX_LOCATIONS)
    {
        return {};
    }

    int previous[MAX_LOCATIONS];
    bool visited[MAX_LOCATIONS] = {false};
    int queue[MAX_LOCATIONS];
    int front = 0;
    int rear = 0;

    for(int i = 0; i < MAX_LOCATIONS; i++)
    {
        previous[i] = -1;
    }

    queue[rear++] = source;
    visited[source] = true;

    while(front < rear)
    {
        int current = queue[front++];

        if(current == destination)
        {
            std::vector<int> route;
            for(int location = destination; location != -1; location = previous[location])
            {
                route.push_back(location);
            }

            std::reverse(route.begin(), route.end());
            return route;
        }

        for(int i = 0; i < locationCount; i++)
        {
            // BFS follows only open roads.
            if(road[current][i] == 1 && !visited[i])
            {
                visited[i] = true;
                previous[i] = current;
                queue[rear++] = i;
            }
        }
    }

    return {};
}


// ==========================================
// CHECK REACHABILITY
// ==========================================

void checkReachability()
{
    string source;
    string destination;


    cout << "\n========================================\n";
    cout << "          CHECK REACHABILITY\n";
    cout << "========================================\n";


    clearInput();


    cout << "\nEnter source location: ";
    getline(cin, source);


    cout << "Enter destination location: ";
    getline(cin, destination);


    int s = findLocation(source);

    int d = findLocation(destination);


    if(s == -1 || d == -1)
    {
        cout << "\nOne or both locations do not exist.\n";
        return;
    }


    if(isReachable(s, d))
    {
        cout << "\nLocation is REACHABLE through open roads.\n";
    }
    else
    {
        cout << "\nLocation is NOT REACHABLE through open roads.\n";
    }
}


// ==========================================
// LOCATION MANAGEMENT MENU
// ==========================================

void locationMenu()
{
    int choice;


    do
    {
        cout << "\n========================================\n";
        cout << "          LOCATION MANAGEMENT\n";
        cout << "========================================\n";


        cout << "\n1. Enter Location\n";
        cout << "2. Display Location Hierarchy\n";
        cout << "3. Add Road\n";
        cout << "4. Block Road\n";
        cout << "5. Open Road\n";
        cout << "6. Display Road Network\n";
        cout << "7. Check Reachability using BFS\n";
        cout << "8. Exit Location Module\n";


        cout << "\nEnter choice: ";
        cin >> choice;


        switch(choice)
        {
            case 1:
                enterLocation();
                break;


            case 2:
                displayHierarchy();
                break;


            case 3:
                addRoad();
                break;


            case 4:
                blockRoad();
                break;


            case 5:
                openRoad();
                break;


            case 6:
                displayRoadNetwork();
                break;


            case 7:
                checkReachability();
                break;


            case 8:
                cout << "\nReturning to Main Menu...\n";
                break;


            default:
                cout << "\nInvalid choice.\n";
        }


    } while(choice != 8);
}